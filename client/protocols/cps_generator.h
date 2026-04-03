#ifndef CPS_GENERATOR_H
#define CPS_GENERATOR_H

#include <QString>
#include <QRandomGenerator>
#include <QDateTime>

namespace amnezia {

class CpsGenerator {
public:
    enum class Preset {
        QUIC, DNS, SIP, Telegram, Russia2026, Iran
    };

    static QString generateCps(Preset preset);
    static QString autoSelectCpsForCountry(const QString& countryCode); // РФ, IR и т.д.
};

} // namespace amnezia

#endif // CPS_GENERATOR_H