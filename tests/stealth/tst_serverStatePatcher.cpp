#include <QtTest>

#include "core/models/containerConfig.h"
#include "core/models/protocols/awgProtocolConfig.h"
#include "core/models/protocols/wireGuardProtocolConfig.h"
#include "core/utils/selfhosted/serverStatePatcher.h"

using namespace amnezia;

namespace
{
    const char *awgServerConfig = "[Interface]\n"
                                  "PrivateKey = SERVERKEY\n"
                                  "Address = 10.8.1.0/24\n"
                                  "ListenPort = 55424\n"
                                  "Jc = 4\n"
                                  "S1 = 12\n"
                                  "S3 = 12\n"
                                  "H1 = 1\n"
                                  "# I1 = <r 2>\n"
                                  "\n"
                                  "[Peer]\n"
                                  "PublicKey = PEER1\n"
                                  "AllowedIPs = 10.8.1.1/32\n"
                                  "\n"
                                  "[Peer]\n"
                                  "PublicKey = PEER2\n"
                                  "AllowedIPs = 10.8.1.2/32\n";

    ContainerConfig awgConfig(const std::function<void(AwgServerConfig &)> &edit)
    {
        AwgProtocolConfig awg;
        awg.serverConfig.port = "40000";
        awg.serverConfig.junkPacketCount = "5";
        awg.serverConfig.initPacketJunkSize = "24";
        awg.serverConfig.initPacketMagicHeader = "1";
        awg.serverConfig.headerProtectionKey = "HPKEY";
        awg.serverConfig.randomTrailers = "off";
        edit(awg.serverConfig);

        ContainerConfig config;
        config.container = DockerContainer::Awg2;
        config.protocolConfig = awg;
        return config;
    }
}

class TestServerStatePatcher : public QObject
{
    Q_OBJECT

private slots:
    void keepsKeysAndPeers()
    {
        bool ok = false;
        const QString out = QString::fromUtf8(serverstate::patchWireguardServerConfig(
                DockerContainer::Awg2, awgServerConfig, awgConfig([](AwgServerConfig &) {}), ok));

        QVERIFY(ok);
        QVERIFY(out.contains("PrivateKey = SERVERKEY"));
        QVERIFY(out.contains("Address = 10.8.1.0/24"));
        QCOMPARE(out.count("[Peer]"), 2);
        QVERIFY(out.contains("PublicKey = PEER2\nAllowedIPs = 10.8.1.2/32"));
    }

    void appliesServerSettingsInsideInterface()
    {
        bool ok = false;
        const QString out = QString::fromUtf8(serverstate::patchWireguardServerConfig(
                DockerContainer::Awg2, awgServerConfig, awgConfig([](AwgServerConfig &) {}), ok));

        QVERIFY(out.contains("ListenPort = 40000"));
        QVERIFY(!out.contains("ListenPort = 55424"));
        QVERIFY(out.contains("Jc = 5"));
        QVERIFY(out.contains("S1 = 24"));
        // AWG 2 -> 3.1 migration adds the header protection key next to the existing peers
        QVERIFY(out.contains("HeaderProtectionKey = HPKEY"));
        QVERIFY(out.indexOf("HeaderProtectionKey") < out.indexOf("[Peer]"));
        // empty values and disabled toggles are not written, like configure_container.sh does
        QVERIFY(!out.contains("S3 ="));
        QVERIFY(!out.contains("RandomTrailers"));
        QVERIFY(!out.contains("# I1"));
    }

    void legacyAwgGetsNoAwg3Keys()
    {
        ContainerConfig config = awgConfig([](AwgServerConfig &s) { s.cookieReplyPacketJunkSize = "30"; });
        config.container = DockerContainer::Awg;

        bool ok = false;
        const QString out =
                QString::fromUtf8(serverstate::patchWireguardServerConfig(DockerContainer::Awg, awgServerConfig, config, ok));
        QVERIFY(ok);
        QVERIFY(!out.contains("HeaderProtectionKey"));
        QVERIFY(out.contains("S3 = 12")); // untouched: not a key of the legacy server
    }

    void wireguardPatchesOnlyPort()
    {
        WireGuardProtocolConfig wg;
        wg.serverConfig.port = "7777";
        ContainerConfig config;
        config.container = DockerContainer::WireGuard;
        config.protocolConfig = wg;

        bool ok = false;
        const QString out =
                QString::fromUtf8(serverstate::patchWireguardServerConfig(DockerContainer::WireGuard, awgServerConfig, config, ok));
        QVERIFY(ok);
        QVERIFY(out.contains("ListenPort = 7777"));
        QVERIFY(out.contains("Jc = 4"));
    }

    void rejectsMalformedConfig()
    {
        bool ok = true;
        serverstate::patchWireguardServerConfig(DockerContainer::Awg2, "", awgConfig([](AwgServerConfig &) {}), ok);
        QVERIFY(!ok);
    }

    void preservesStateUnlessSubnetMoves()
    {
        const ContainerConfig base = awgConfig([](AwgServerConfig &) {});
        QVERIFY(serverstate::canPreserveState(DockerContainer::Awg2, base,
                                              awgConfig([](AwgServerConfig &s) { s.port = "1"; })));
        QVERIFY(!serverstate::canPreserveState(DockerContainer::Awg2, base,
                                               awgConfig([](AwgServerConfig &s) { s.subnetAddress = "10.9.0.0"; })));
        QVERIFY(serverstate::canPreserveState(DockerContainer::Xray, {}, {}));
        QVERIFY(!serverstate::canPreserveState(DockerContainer::OpenVpn, {}, {}));
    }
};

QTEST_APPLESS_MAIN(TestServerStatePatcher)
#include "tst_serverStatePatcher.moc"
