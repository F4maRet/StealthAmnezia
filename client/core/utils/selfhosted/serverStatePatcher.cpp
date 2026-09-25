#include "serverStatePatcher.h"

#include <QRegularExpression>

#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/utils/constants/configKeys.h"
#include "core/utils/constants/protocolConstants.h"
#include "core/utils/containers/containerUtils.h"

namespace amnezia::serverstate
{
    namespace
    {
        bool isWireguardLike(DockerContainer container)
        {
            return ContainerUtils::isAwgContainer(container) || container == DockerContainer::WireGuard;
        }

        QString subnetOf(DockerContainer container, const ContainerConfig &config)
        {
            QString subnet;
            if (const auto *awg = config.getAwgProtocolConfig(); awg && ContainerUtils::isAwgContainer(container)) {
                subnet = awg->serverConfig.subnetAddress;
            } else if (const auto *wg = config.getWireGuardProtocolConfig()) {
                subnet = wg->serverConfig.subnetAddress;
            }
            return subnet.isEmpty() ? QString(protocols::wireguard::defaultSubnetAddress) : subnet;
        }

        QString toggle(const QString &value)
        {
            return AwgProtocolConfig::isToggleEnabled(value) ? value : QString();
        }
    }

    bool canPreserveState(DockerContainer container, const ContainerConfig &oldConfig, const ContainerConfig &newConfig)
    {
        if (isWireguardLike(container)) {
            // peers are addressed inside the subnet, moving it would orphan every issued config
            return subnetOf(container, oldConfig) == subnetOf(container, newConfig);
        }
        return container == DockerContainer::Xray;
    }

    QString wireguardServerConfigPath(DockerContainer container)
    {
        switch (container) {
        case DockerContainer::Awg2: return protocols::awg::serverConfigPath;
        case DockerContainer::Awg: return protocols::awg::serverLegacyConfigPath;
        case DockerContainer::WireGuard: return protocols::wireguard::serverConfigPath;
        default: return QString();
        }
    }

    QString healthCheckCommand(DockerContainer container)
    {
        switch (container) {
        case DockerContainer::Awg2: return QStringLiteral("awg show awg0");
        case DockerContainer::Awg:
        case DockerContainer::WireGuard: return QStringLiteral("wg show wg0");
        case DockerContainer::Xray:
        case DockerContainer::SSXray: return QStringLiteral("pgrep -x xray");
        default: return QStringLiteral("true");
        }
    }

    QList<QPair<QString, QString>> wireguardInterfaceValues(DockerContainer container, const ContainerConfig &config)
    {
        QList<QPair<QString, QString>> values;

        if (container == DockerContainer::WireGuard) {
            const auto *wg = config.getWireGuardProtocolConfig();
            const QString port = wg ? wg->serverConfig.port : QString();
            values.append({ "ListenPort", port.isEmpty() ? QString(protocols::wireguard::defaultPort) : port });
            return values;
        }

        const auto *awg = config.getAwgProtocolConfig();
        if (!awg) {
            return values;
        }
        const AwgServerConfig &s = awg->serverConfig;

        // mirrors configure_container.sh: every AWG parameter is optional and an empty one is not written
        values.append({ "ListenPort", s.port.isEmpty() ? QString(protocols::awg::defaultPort) : s.port });
        values.append({ configKey::junkPacketCount, s.junkPacketCount });
        values.append({ configKey::junkPacketMinSize, s.junkPacketMinSize });
        values.append({ configKey::junkPacketMaxSize, s.junkPacketMaxSize });
        values.append({ configKey::initPacketJunkSize, s.initPacketJunkSize });
        values.append({ configKey::responsePacketJunkSize, s.responsePacketJunkSize });
        values.append({ configKey::initPacketMagicHeader, s.initPacketMagicHeader });
        values.append({ configKey::responsePacketMagicHeader, s.responsePacketMagicHeader });
        values.append({ configKey::underloadPacketMagicHeader, s.underloadPacketMagicHeader });
        values.append({ configKey::transportPacketMagicHeader, s.transportPacketMagicHeader });

        if (container == DockerContainer::Awg) {
            // the legacy AmneziaWG server knows nothing past H1-H4
            return values;
        }

        values.append({ configKey::cookieReplyPacketJunkSize, s.cookieReplyPacketJunkSize });
        values.append({ configKey::transportPacketJunkSize, s.transportPacketJunkSize });
        values.append({ configKey::headerProtectionKey, s.headerProtectionKey });
        values.append({ configKey::contentPaddingAddition, s.contentPaddingAddition });
        values.append({ configKey::rekeyAfterTime, s.rekeyAfterTime });
        values.append({ configKey::rekeyTimeout, s.rekeyTimeout });
        values.append({ configKey::rejectAfterTime, s.rejectAfterTime });
        values.append({ configKey::keepaliveTimeout, s.keepaliveTimeout });
        values.append({ configKey::maxHandshakeAttempts, s.maxHandshakeAttempts });
        values.append({ configKey::randomTrailers, toggle(s.randomTrailers) });
        values.append({ configKey::disableCookies, toggle(s.disableCookies) });

        values.append({ QString("# ") + configKey::specialJunk1, s.specialJunk1 });
        values.append({ QString("# ") + configKey::specialJunk2, s.specialJunk2 });
        values.append({ QString("# ") + configKey::specialJunk3, s.specialJunk3 });
        values.append({ QString("# ") + configKey::specialJunk4, s.specialJunk4 });
        values.append({ QString("# ") + configKey::specialJunk5, s.specialJunk5 });
        return values;
    }

    QString patchWireguardInterface(const QString &serverConfig, const QList<QPair<QString, QString>> &values)
    {
        static const QRegularExpression sectionRe(QStringLiteral(R"(^\s*\[(\w+)\]\s*$)"));
        static const QRegularExpression keyRe(QStringLiteral(R"(^\s*(#\s*)?([A-Za-z0-9]+)\s*=)"));

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
                const auto match = keyRe.match(line);
                if (match.hasMatch()) {
                    const QString key = (match.captured(1).isEmpty() ? QString() : QStringLiteral("# ")) + match.captured(2);
                    auto it = std::find_if(values.cbegin(), values.cend(),
                                           [&key](const QPair<QString, QString> &v) { return v.first == key; });
                    if (it != values.cend()) {
                        if (!it->second.isEmpty() && !applied.contains(it->first)) {
                            result.append(QString("%1 = %2").arg(it->first, it->second));
                        }
                        applied.append(it->first);
                        continue;
                    }
                }
            }
            result.append(line);
        }

        if (inInterface) {
            appendMissing();
        }

        return result.join('\n');
    }

    QByteArray patchWireguardServerConfig(DockerContainer container, const QByteArray &serverConfig, const ContainerConfig &config,
                                          bool &ok)
    {
        ok = isWireguardLike(container) && serverConfig.contains("[Interface]") && serverConfig.contains("PrivateKey");
        if (!ok) {
            return serverConfig;
        }
        return patchWireguardInterface(QString::fromUtf8(serverConfig), wireguardInterfaceValues(container, config)).toUtf8();
    }
}
