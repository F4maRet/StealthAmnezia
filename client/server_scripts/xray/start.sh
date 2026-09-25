#!/bin/bash

# This scripts copied from Amnezia client to Docker container to /opt/amnezia and launched every time container starts

echo "Container startup"
#ifconfig eth0:0 $SERVER_IP_ADDRESS netmask 255.255.255.255 up

# Idempotent iptables: add the rule only if it is not present yet,
# so container restarts do not pile up duplicate rules
ipt() { iptables -C "$@" 2>/dev/null || iptables -A "$@"; }
ip6t() { ip6tables -C "$@" 2>/dev/null || ip6tables -A "$@"; }

ipt INPUT -i lo -j ACCEPT
ipt INPUT -m conntrack --ctstate ESTABLISHED,RELATED -j ACCEPT
ipt INPUT -p icmp -j ACCEPT
ipt INPUT -p tcp --dport 80 -j ACCEPT
ipt INPUT -p tcp --dport 443 -j ACCEPT
ipt INPUT -p tcp --dport $XRAY_SERVER_PORT -j ACCEPT
ipt INPUT -p udp --dport $XRAY_SERVER_PORT -j ACCEPT
iptables -P INPUT DROP

ip6t INPUT -i lo -j ACCEPT
ip6t INPUT -m state --state RELATED,ESTABLISHED -j ACCEPT
ip6t INPUT -p ipv6-icmp -j ACCEPT
ip6tables -P INPUT DROP

# Supervisor: restarts xray if it crashes and lets clients reload the config
# with "pkill -x xray" instead of restarting the whole container.
# Only the newest start.sh instance supervises; an older one steps aside.
SUPERVISOR_PID_FILE=/opt/amnezia/.xray-supervisor.pid
echo $$ > $SUPERVISOR_PID_FILE
touch /opt/amnezia/.xray-supervised

# kill daemons in case of restart
killall -KILL xray 2>/dev/null

while [ "$(cat $SUPERVISOR_PID_FILE 2>/dev/null)" = "$$" ]; do
    if [ -f /opt/amnezia/xray/server.json ]; then
        xray -config /opt/amnezia/xray/server.json
    fi
    sleep 1
done

tail -f /dev/null
