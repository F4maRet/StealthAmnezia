#ifndef QML_REGISTER_PROTOCOLS_H
#define QML_REGISTER_PROTOCOLS_H

#include <QQmlEngine>

#include "protocols_defs.h"
#include "awgprotocol.h"
#include "xrayprotocol.h"

inline void registerProtocolsTypes()
{
    qmlRegisterType<amnezia::AwgProtocol>("Amnezia", 1, 0, "AwgProtocol");
    qmlRegisterType<amnezia::XrayProtocol>("Amnezia", 1, 0, "XrayProtocol");

    // StealthAmnezia: только эти два протокола
    qRegisterMetaType<amnezia::config_key::Awg>("AwgConfig");
    qRegisterMetaType<amnezia::config_key::Xray>("XrayConfig");

    qmlRegisterType<amnezia::HybridProtocol>("Amnezia", 1, 0, "HybridProtocol");
    qRegisterMetaType<amnezia::HybridProtocol*>("HybridProtocol*");
}

#endif // QML_REGISTER_PROTOCOLS_H