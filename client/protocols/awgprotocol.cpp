#include "awgprotocol.h"
#include "cps_generator.h"
#include "protocols_defs.h"

Awg::Awg(const QJsonObject &configuration, QObject *parent)
    : WireguardProtocol(configuration, parent)
{
    // StealthAmnezia: авто-CPS
    QJsonObject cfg = configuration; // копия, т.к. оригинал const
    QString country = cfg.value("country").toString().toUpper();
    QString cps = CpsGenerator::autoForCountry(country);

    QJsonObject awgConfig = cfg.value(config_key::awg).toObject();
    awgConfig[config_key::specialJunk1] = cps;
    awgConfig[config_key::specialJunk2] = cps;
    cfg[config_key::awg] = awgConfig;

    // Передаём обновлённый конфиг в базовый класс
    setConfiguration(cfg); // если есть такой метод в WireguardProtocol
}