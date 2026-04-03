#include "hybrid_connection_manager.h"

namespace amnezia {

HybridConnectionManager::HybridConnectionManager(QObject *parent)
    : QObject(parent)
{
    m_hybrid = new HybridProtocol(QJsonObject(), this); // config передаётся позже
}

void HybridConnectionManager::connectToServer(const QJsonObject &config)
{
    m_hybrid = new HybridProtocol(config, this);
    connect(m_hybrid, &HybridProtocol::connected, this, [this]() {
        emit connectionStateChanged(true, "Hybrid (AWG/XRay)");
    });
    connect(m_hybrid, &HybridProtocol::disconnected, this, [this]() {
        emit connectionStateChanged(false, "Hybrid");
    });
    m_hybrid->start();
}

void HybridConnectionManager::disconnect()
{
    if (m_hybrid) m_hybrid->stop();
}

bool HybridConnectionManager::isConnected() const
{
    return m_hybrid && m_hybrid->isConnected();
}

} // namespace amnezia