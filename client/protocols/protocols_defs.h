#ifndef PROTOCOLS_DEFS_H
#define PROTOCOLS_DEFS_H

#include <QDebug>
#include <QMetaEnum>
#include <QObject>

namespace amnezia
{
    namespace config_key
    {
        // Общие ключи конфига
        constexpr char hostName[] = "hostName";
        constexpr char port[] = "port";
        constexpr char local_port[] = "local_port";
        constexpr char dns1[] = "dns1";
        constexpr char dns2[] = "dns2";
        constexpr char description[] = "description";
        constexpr char name[] = "name";
        constexpr char config[] = "config";
        constexpr char vpnproto[] = "protocol";
        constexpr char protocols[] = "protocols";

        // Ключи AmneziaWG (CPS-обфускация)
        constexpr char junkPacketCount[] = "Jc";
        constexpr char junkPacketMinSize[] = "Jmin";
        constexpr char junkPacketMaxSize[] = "Jmax";
        constexpr char initPacketJunkSize[] = "S1";
        constexpr char responsePacketJunkSize[] = "S2";
        constexpr char cookieReplyPacketJunkSize[] = "S3";
        constexpr char transportPacketJunkSize[] = "S4";
        constexpr char initPacketMagicHeader[] = "H1";
        constexpr char responsePacketMagicHeader[] = "H2";
        constexpr char underloadPacketMagicHeader[] = "H3";
        constexpr char transportPacketMagicHeader[] = "H4";
        constexpr char specialJunk1[] = "I1";
        constexpr char specialJunk2[] = "I2";
        constexpr char specialJunk3[] = "I3";
        constexpr char specialJunk4[] = "I4";
        constexpr char specialJunk5[] = "I5";

        // Ключи XRay Reality
        constexpr char site[] = "site";
        constexpr char client_priv_key[] = "client_priv_key";
        constexpr char client_pub_key[] = "client_pub_key";

        constexpr char awg[] = "awg";
        constexpr char xray[] = "xray";

        constexpr char splitTunnelSites[] = "splitTunnelSites";
        constexpr char splitTunnelType[] = "splitTunnelType";
        constexpr char splitTunnelApps[] = "splitTunnelApps";
        constexpr char appSplitTunnelType[] = "appSplitTunnelType";
        constexpr char allowedDnsServers[] = "allowedDnsServers";
        constexpr char killSwitchOption[] = "killSwitchOption";
    }

    namespace protocols
    {
        namespace dns
        {
            constexpr char amneziaDnsIp[] = "172.29.172.254";
        }

        namespace awg
        {
            constexpr char defaultPort[] = "51820";
            constexpr char defaultMtu[] = "1376";
        }

        namespace xray
        {
            constexpr char defaultPort[] = "443";
            constexpr char defaultLocalProxyPort[] = "10808";
            constexpr char defaultSite[] = "www.google.com";
        }
    }
}

#endif // PROTOCOLS_DEFS_H