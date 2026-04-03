#ifndef HYBRIDPROTOCOL_H
#define HYBRIDPROTOCOL_H

#include <QObject>
#include <QTimer>
#include "awgprotocol.h"
#include "xrayprotocol.h"

namespace amnezia {

class HybridProtocol : public QObject
{
    Q_OBJECT

public:
    explicit HybridProtocol(const QJsonObject &config, QObject *parent = nullptr);
    ~HybridProtocol() override;

    void start();
    void stop();
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void fallbackTriggered(const QString &fromProtocol, const QString &toProtocol);

private slots:
    void onPrimaryDisconnected();
    void onSecondaryDisconnected();
    void checkConnection();

private:
    Awg* m_primary;      // AmneziaWG — первый
    XrayProtocol* m_secondary; // XRay — fallback
    QTimer* m_fallbackTimer;
    bool m_usingSecondary = false;
    int m_fallbackAttempts = 0;
};

} // namespace amnezia

#endif // HYBRIDPROTOCOL_H