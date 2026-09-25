#include "serverStatePatcher.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QRegularExpression>

#include "protocols/protocols_defs.h"

namespace amnezia::serverstate
{
    namespace
    {
        QJsonObject protoConfig(DockerContainer container, const QJsonObject &config)
        {
            return config.value(ProtocolProps::protoToString(ContainerProps::defaultProtocol(container))).toObject();
        }

        bool isWireguardLike(DockerContainer container)
        {
            return ContainerProps::isAwgContainer(container) || container == DockerContainer::WireGuard;
        }
    }

    bool canPreserveState(DockerContainer container, const QJsonObject &oldConfig, const QJsonObject &newConfig)
    {
        if (isWireguardLike(container)) {
            // peers are addressed inside the subnet, moving it would orphan every issued config
            const QString defaultSubnet = protocols::wireguard::defaultSubnetAddress;
            return protoConfig(container, oldConfig).value(config_key::subnet_address).toString(defaultSubnet)
                    == protoConfig(container, newConfig).value(config_key::subnet_address).toString(defaultSubnet);
        }
        return container == DockerContainer::Xray;
    }

    QString serverConfigRelativePath(DockerContainer container)
    {
        switch (container) {
        case DockerContainer::Awg2: return QStringLiteral("awg/awg0.conf");
        case DockerContainer::Awg: return QStringLiteral("awg/wg0.conf");
        case DockerContainer::WireGuard: return QStringLiteral("wireguard/wg0.conf");
        case DockerContainer::Xray: return QStringLiteral("xray/server.json");
        default: return QString();
        }
    }

    QString healthCheckCommand(DockerContainer container)
    {
        switch (container) {
        case DockerContainer::Awg2: return QStringLiteral("awg show awg0");
        case DockerContainer::Awg:
        case DockerContainer::WireGuard: return QStringLiteral("wg show wg0");
        case DockerContainer::Xray: return QStringLiteral("pgrep -x xray");
        default: return QStringLiteral("true");
        }
    }

    QList<QPair<QString, QString>> wireguardInterfaceValues(DockerContainer container, const QJsonObject &config)
    {
        const QJsonObject proto = protoConfig(container, config);
        QList<QPair<QString, QString>> values;

        if (container == DockerContainer::WireGuard) {
            values.append({ "ListenPort", proto.value(config_key::port).toString(protocols::wireguard::defaultPort) });
            return values;
        }

        // a key missing in the app config keeps the value that is on the server (null string), only the port has a
        // well known default the server was installed with
        auto value = [&proto](const char *key) { return proto.contains(key) ? proto.value(key).toString("") : QString(); };

        values.append({ "ListenPort", proto.value(config_key::port).toString(protocols::awg::defaultPort) });
        values.append({ "Jc", value(config_key::junkPacketCount) });
        values.append({ "Jmin", value(config_key::junkPacketMinSize) });
        values.append({ "Jmax", value(config_key::junkPacketMaxSize) });
        values.append({ "S1", value(config_key::initPacketJunkSize) });
        values.append({ "S2", value(config_key::responsePacketJunkSize) });
        if (container == DockerContainer::Awg2) {
            // S3/S4 are not known to the legacy AmneziaWG server
            values.append({ "S3", value(config_key::cookieReplyPacketJunkSize) });
            values.append({ "S4", value(config_key::transportPacketJunkSize) });
        }
        values.append({ "H1", value(config_key::initPacketMagicHeader) });
        values.append({ "H2", value(config_key::responsePacketMagicHeader) });
        values.append({ "H3", value(config_key::underloadPacketMagicHeader) });
        values.append({ "H4", value(config_key::transportPacketMagicHeader) });
        return values;
    }

    QString patchWireguardInterface(const QString &serverConfig, const QList<QPair<QString, QString>> &values)
    {
        static const QRegularExpression sectionRe(QStringLiteral(R"(^\s*\[(\w+)\]\s*$)"));
        static const QRegularExpression keyRe(QStringLiteral(R"(^\s*([A-Za-z0-9]+)\s*=)"));

        const QStringList lines = serverConfig.split('\n');
        QStringList result;
        QStringList applied;
        bool inInterface = false;

        auto appendMissing = [&]() {
            for (const auto &value : values) {
                if (!applied.contains(value.first) && !value.second.isEmpty()) {
                    result.append(QString("%1 = %2").arg(value.first, value.second));
                    applied.append(value.first);
                }
            }
        };

        for (const QString &line : lines) {
            const auto section = sectionRe.match(line);
            if (section.hasMatch()) {
                if (inInterface) {
                    appendMissing();
                }
                inInterface = section.captured(1) == QLatin1String("Interface");
                result.append(line);
                continue;
            }

            if (inInterface) {
                const auto key = keyRe.match(line);
                auto it = std::find_if(values.cbegin(), values.cend(),
                                       [&key](const QPair<QString, QString> &v) { return key.hasMatch() && v.first == key.captured(1); });
                if (it != values.cend() && !it->second.isNull()) {
                    if (!it->second.isEmpty() && !applied.contains(it->first)) {
                        result.append(QString("%1 = %2").arg(it->first, it->second));
                    }
                    applied.append(it->first);
                    continue;
                }
            }
            result.append(line);
        }

        if (inInterface) {
            appendMissing();
        }

        return result.join('\n');
    }

    QByteArray patchXrayServerConfig(const QByteArray &serverConfig, const QString &port, const QString &site, bool &ok)
    {
        ok = false;
        QJsonObject root = QJsonDocument::fromJson(serverConfig).object();
        QJsonArray inbounds = root.value("inbounds").toArray();
        if (inbounds.isEmpty()) {
            return serverConfig;
        }

        QJsonObject inbound = inbounds.at(0).toObject();
        if (!inbound.value("settings").toObject().contains("clients")) {
            return serverConfig;
        }

        inbound["port"] = port.toInt();

        QJsonObject stream = inbound.value("streamSettings").toObject();
        QJsonObject reality = stream.value("realitySettings").toObject();
        reality["dest"] = site + ":443";

        QJsonArray serverNames { site };
        for (const QJsonValue &name : reality.value("serverNames").toArray()) {
            if (!serverNames.contains(name)) {
                serverNames.append(name);
            }
        }
        reality["serverNames"] = serverNames;

        stream["realitySettings"] = reality;
        inbound["streamSettings"] = stream;
        inbounds[0] = inbound;
        root["inbounds"] = inbounds;

        ok = true;
        return QJsonDocument(root).toJson();
    }

    QByteArray patchServerConfig(DockerContainer container, const QByteArray &serverConfig, const QJsonObject &containerConfig, bool &ok)
    {
        if (container == DockerContainer::Xray) {
            const QJsonObject proto = protoConfig(container, containerConfig);
            return patchXrayServerConfig(serverConfig, proto.value(config_key::port).toString(protocols::xray::defaultPort),
                                         proto.value(config_key::site).toString(protocols::xray::defaultSite), ok);
        }

        ok = isWireguardLike(container) && serverConfig.contains("[Interface]");
        if (!ok) {
            return serverConfig;
        }
        return patchWireguardInterface(QString::fromUtf8(serverConfig), wireguardInterfaceValues(container, containerConfig)).toUtf8();
    }
}
