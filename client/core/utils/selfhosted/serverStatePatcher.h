#ifndef SERVERSTATEPATCHER_H
#define SERVERSTATEPATCHER_H

#include <QByteArray>
#include <QList>
#include <QPair>
#include <QString>

#include "core/models/containerConfig.h"
#include "core/utils/containerEnum.h"

// Applies changed server settings to the state already stored on the server instead of regenerating it.
// Keys, peers and xray clients stay untouched, so existing users keep their configs.
namespace amnezia::serverstate
{
    // Server settings of these containers can be changed without losing users
    bool canPreserveState(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig);

    // Path of the (amnezia)wireguard server config inside the container, empty for other containers
    QString wireguardServerConfigPath(DockerContainer container);

    // Shell command run inside the container that succeeds only when the protocol is up
    QString healthCheckCommand(DockerContainer container);

    // [Interface] values of a (amnezia)wireguard server config; an empty value removes the key. Keys prefixed with
    // "# " are the commented I1-I5 lines the server keeps for the app to read back
    QList<QPair<QString, QString>> wireguardInterfaceValues(DockerContainer container, const ContainerConfig &config);

    // Replaces the given keys in the [Interface] section only, leaving PrivateKey, Address and all [Peer] sections as is
    QString patchWireguardInterface(const QString &serverConfig, const QList<QPair<QString, QString>> &values);

    // Applies the settings of config to a (amnezia)wireguard server config, ok is false for a malformed config
    QByteArray patchWireguardServerConfig(DockerContainer container, const QByteArray &serverConfig, const ContainerConfig &config,
                                          bool &ok);
}

#endif // SERVERSTATEPATCHER_H
