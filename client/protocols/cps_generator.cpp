#include "cps_generator.h"

namespace amnezia {

QString CpsGenerator::generateCps(Preset preset) {
    auto rand = QRandomGenerator::global();
    QStringList tags;

    // Базовый заголовок
    tags << QString("<b 0x%1>").arg(rand->bounded(0xC7000000, 0xC7FFFFFF), 8, 16, QChar('0'));

    // Timestamp
    tags << "<t>";

    // Random junk
    tags << QString("<r %1>").arg(20 + rand->bounded(80));

    // Alphanumeric
    tags << QString("<rc %1>").arg(8 + rand->bounded(16));

    // Дополнительный random digits (иногда)
    if (rand->bounded(2) == 0) {
        tags << QString("<rd 4>");
    }

    // Adaptive по пресету
    if (preset == Preset::Russia2026 || preset == Preset::Iran) {
        tags << "<I1><I3>"; // усиленная обфускация
    }

    return tags.join("");
}

QString CpsGenerator::autoSelectCpsForCountry(const QString& countryCode) {
    if (countryCode == "RU" || countryCode == "BY") return generateCps(Preset::Russia2026);
    if (countryCode == "IR" || countryCode == "CN") return generateCps(Preset::Iran);
    return generateCps(Preset::QUIC); // default
}

} // namespace amnezia