#ifndef SERVERSTATEPATCHER_H
#define SERVERSTATEPATCHER_H

#include <QByteArray>
#include <QJsonObject>
#include <QList>
#include <QPair>
#include <QString>

#include "containers/containers_defs.h"

// Applies changed server settings to the state already stored on the server instead of regenerating it.
// Keys, peers and xray clients stay untouched, so existing users keep their configs.
namespace amnezia::serverstate
{
    // Server settings of these containers can be changed without losing users
    bool canPreserveState(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig);

    // Path of the file patched by patchServerConfig(), relative to /opt/amnezia inside the container
    QString serverConfigRelativePath(DockerContainer container);

    // Shell command run inside the container that succeeds only when the protocol is up
    QString healthCheckCommand(DockerContainer container);

    // [Interface] values of a (amnezia)wireguard server config; an empty value removes the key, a null one keeps it
    QList<QPair<QString, QString>> wireguardInterfaceValues(DockerContainer container, const QJsonObject &config);

    // Replaces the given keys in the [Interface] section only, leaving PrivateKey, Address and all [Peer] sections as is
    QString patchWireguardInterface(const QString &serverConfig, const QList<QPair<QString, QString>> &values);

    // Updates the listen port and the reality site. The previous site stays in serverNames so already issued
    // client configs keep connecting after the site is changed
    QByteArray patchXrayServerConfig(const QByteArray &serverConfig, const QString &port, const QString &site, bool &ok);

    // Applies the settings from containerConfig to the server config of the container
    QByteArray patchServerConfig(DockerContainer container, const QByteArray &serverConfig, const QJsonObject &containerConfig, bool &ok);
}

#endif // SERVERSTATEPATCHER_H
