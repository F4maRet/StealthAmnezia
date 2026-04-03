#include "hybridprotocol.h"
#include "cps_generator.h"
#include "protocols_defs.h"

namespace amnezia {

HybridProtocol::HybridProtocol(const QJsonObject &config, QObject *parent)
    : QObject(parent)
{
    m_primary = new Awg(config, this);
    m_secondary = new XrayProtocol(config, this);

    m_fallbackTimer = new QTimer(this);
    m_fallbackTimer->setSingleShot(true);
    connect(m_fallbackTimer, &QTimer::timeout, this, &HybridProtocol::checkConnection);

    connect(m_primary, &Awg::connected, this, &HybridProtocol::connected);
    connect(m_primary, &Awg::disconnected, this, &HybridProtocol::onPrimaryDisconnected);

    connect(m_secondary, &XrayProtocol::connected, this, &HybridProtocol::connected);
    connect(m_secondary, &XrayProtocol::disconnected, this, &HybridProtocol::onSecondaryDisconnected);
}

HybridProtocol::~HybridProtocol()
{
    stop();
}

void HybridProtocol::start()
{
    m_usingSecondary = false;
    m_fallbackAttempts = 0;
    m_primary->start();
    m_fallbackTimer->start(8000); // 8 секунд на попытку подключения
}

void HybridProtocol::stop()
{
    m_fallbackTimer->stop();
    if (m_usingSecondary) {
        m_secondary->stop();
    } else {
        m_primary->stop();
    }
}

bool HybridProtocol::isConnected() const
{
    return m_usingSecondary ? m_secondary->isConnected() : m_primary->isConnected();
}

void HybridProtocol::onPrimaryDisconnected()
{
    if (m_fallbackAttempts >= 2) {
        emit disconnected();
        return;
    }

    qDebug() << "Hybrid fallback: AWG упал → переключаемся на XRay";
    m_primary->stop();
    m_usingSecondary = true;
    m_fallbackAttempts++;
    emit fallbackTriggered("AmneziaWG", "XRay");

    // Генерируем свежий CPS для следующей попытки AWG
    QString cps = CpsGenerator::autoSelectCpsForCountry("RU"); // можно брать из config
    m_primary->setCps(cps); // если добавишь метод в Awg

    m_secondary->start();
    m_fallbackTimer->start(12000); // XRay чуть дольше
}

void HybridProtocol::onSecondaryDisconnected()
{
    qDebug() << "Hybrid fallback: XRay упал → возвращаемся к AWG";
    m_secondary->stop();
    m_usingSecondary = false;
    m_primary->start();
}

void HybridProtocol::checkConnection()
{
    if (!isConnected()) {
        if (m_usingSecondary) {
            onSecondaryDisconnected();
        } else {
            onPrimaryDisconnected();
        }
    }
}

} // namespace amnezia