#ifndef HYBRID_CONNECTION_MANAGER_H
#define HYBRID_CONNECTION_MANAGER_H

#include <QObject>
#include "hybridprotocol.h"

namespace amnezia {

class HybridConnectionManager : public QObject
{
    Q_OBJECT

public:
    explicit HybridConnectionManager(QObject *parent = nullptr);
    void connectToServer(const QJsonObject &config);
    void disconnect();
    bool isConnected() const;

signals:
    void connectionStateChanged(bool connected, const QString &protocol);

private:
    HybridProtocol* m_hybrid;
};

} // namespace amnezia

#endif // HYBRID_CONNECTION_MANAGER_H