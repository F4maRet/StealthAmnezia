#!/bin/bash
# Transactional in-place upgrade of an Amnezia container.
#
# Rebuilds the image and recreates the container while keeping everything the
# protocol stores in /opt/amnezia inside the container: server keys, peers,
# xray clients, clientsTable. Users keep working without re-issuing configs.
#
#   1. snapshot   /opt/amnezia of the running container -> host
#   2. build      new image as $CONTAINER_NAME:next (old container keeps serving)
#   3. swap       stop old container, keep it as $CONTAINER_NAME-prev
#   4. restore    run new container, copy the snapshot back, apply overlay files
#   5. verify     container runs and the protocol health check passes
#   6. commit     drop the previous container, or roll back to it on any failure
#
# Inputs in $DOCKERFILE_FOLDER/upgrade: run.sh, start.sh, optional overlay/ dir.
# Result marker on the last line: UPGRADE_OK | UPGRADE_FAILED | UPGRADE_ROLLED_BACK

name="$CONTAINER_NAME"
work="$DOCKERFILE_FOLDER/upgrade"
state="/opt/amnezia/.state/$name"
snap="$state/$(date +%Y%m%d-%H%M%S)"
health_check='$UPGRADE_HEALTH_CHECK'

log() { echo "[upgrade] $*"; }

fail() {
    log "failed before swap, nothing changed: $1"
    docker image rm "$name:next" >/dev/null 2>&1
    echo "UPGRADE_FAILED: $1"
    exit 1
}

rollback() {
    log "rolling back: $1"
    docker rm -f "$name" >/dev/null 2>&1
    docker image tag "$name:prev" "$name:latest"
    docker image rm "$name:prev" >/dev/null 2>&1
    docker rename "$name-prev" "$name"
    docker update --restart=always "$name" >/dev/null
    docker start "$name" >/dev/null
    docker network connect amnezia-dns-net "$name" >/dev/null 2>&1
    echo "UPGRADE_ROLLED_BACK: $1"
    exit 2
}

docker inspect "$name" >/dev/null 2>&1 || fail "container $name not found"

log "1/6 snapshot state to $snap"
mkdir -p "$snap" || fail "cannot create $snap"
docker cp "$name:/opt/amnezia/." "$snap/" || fail "cannot copy /opt/amnezia from $name"
ls -1dt "$state"/*/ 2>/dev/null | tail -n +6 | xargs -r rm -rf

log "2/6 build new image"
docker build --pull -t "$name:next" "$DOCKERFILE_FOLDER" || fail "docker build"

log "3/6 swap containers"
docker rm -f "$name-prev" >/dev/null 2>&1
docker image tag "$(docker inspect -f '{{.Image}}' "$name")" "$name:prev" || fail "cannot tag current image"
docker update --restart=no "$name" >/dev/null
docker stop -t 5 "$name" >/dev/null
docker rename "$name" "$name-prev" || { docker start "$name"; fail "cannot rename $name"; }
docker image tag "$name:next" "$name:latest"
docker image rm "$name:next" >/dev/null 2>&1

log "4/6 start new container and restore state"
bash "$work/run.sh" || rollback "run container"
docker cp "$snap/." "$name:/opt/amnezia/" || rollback "restore state"
if [ -d "$work/overlay" ] && [ -n "$(ls -A "$work/overlay")" ]; then
    docker cp "$work/overlay/." "$name:/opt/amnezia/" || rollback "apply overlay"
fi
if [ -s "$work/start.sh" ]; then
    docker cp "$work/start.sh" "$name:/opt/amnezia/start.sh" || rollback "copy start.sh"
    docker exec "$name" chmod a+x /opt/amnezia/start.sh
fi
# the entrypoint runs /opt/amnezia/start.sh, so a restart brings the protocol up
docker restart -t 2 "$name" >/dev/null || rollback "restart"

log "5/6 health check"
healthy=0
for _ in $(seq 1 20); do
    sleep 1
    if [ "$(docker inspect -f '{{.State.Running}}' "$name" 2>/dev/null)" = "true" ] \
        && docker exec "$name" sh -c "$health_check" >/dev/null 2>&1; then
        healthy=1
        break
    fi
done
[ "$healthy" = "1" ] || rollback "health check: $health_check"

log "6/6 commit"
docker rm -f "$name-prev" >/dev/null 2>&1
docker image rm "$name:prev" >/dev/null 2>&1
docker image prune -f >/dev/null 2>&1
echo "UPGRADE_OK: state kept in $snap"
