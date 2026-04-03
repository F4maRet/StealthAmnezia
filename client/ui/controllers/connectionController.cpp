#include "connectionController.h"

#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS) || defined(MACOS_NE)
    #include <QGuiApplication>
#else
    #include <QApplication>
#endif

#include "utilities.h"
#include "core/controllers/vpnConfigurationController.h"
#include "version.h"
#include "protocols/hybridprotocol.h"   // StealthAmnezia

ConnectionController::ConnectionController(const QSharedPointer<ServersModel> &serversModel,
                                           const QSharedPointer<ContainersModel> &containersModel,
                                           const QSharedPointer<ClientManagementModel> &clientManagementModel,
                                           const QSharedPointer<VpnConnection> &vpnConnection,
                                           const std::shared_ptr<Settings> &settings,
                                           QObject *parent)
    : QObject(parent),
      m_serversModel(serversModel),
      m_containersModel(containersModel),
      m_clientManagementModel(clientManagementModel),
      m_vpnConnection(vpnConnection),
      m_settings(settings)
{
    connect(m_vpnConnection.get(), &VpnConnection::connectionStateChanged, this, &ConnectionController::onConnectionStateChanged);
    connect(this, &ConnectionController::connectButtonClicked, this, &ConnectionController::toggleConnection, Qt::QueuedConnection);

    m_state = Vpn::ConnectionState::Disconnected;

    // StealthAmnezia: инициализируем HybridProtocol
    m_hybridProtocol = new HybridProtocol(QJsonObject(), this);
    connect(m_hybridProtocol, &HybridProtocol::connected, this, [this]() {
        m_state = Vpn::ConnectionState::Connected;
        m_isConnected = true;
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connected");
        emit connectionStateChanged();
    });
    connect(m_hybridProtocol, &HybridProtocol::disconnected, this, [this]() {
        m_state = Vpn::ConnectionState::Disconnected;
        m_isConnected = false;
        m_isConnectionInProgress = false;
        m_connectionStateText = tr("Connect");
        emit connectionStateChanged();
    });
    connect(m_hybridProtocol, &HybridProtocol::fallbackTriggered, this, [](const QString &from, const QString &to) {
        qDebug() << "Hybrid fallback triggered:" << from << "→" << to;
    });
}

void ConnectionController::openConnection()
{
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_IOS) && !defined(MACOS_NE)
    if (!Utils::processIsRunning(Utils::executable(SERVICE_NAME, false), true))
    {
        emit connectionErrorOccurred(ErrorCode::AmneziaServiceNotRunning);
        return;
    }
#endif

    int serverIndex = m_serversModel->getDefaultServerIndex();
    QJsonObject serverConfig = m_serversModel->getServerConfig(serverIndex);

    // StealthAmnezia: используем HybridProtocol вместо старого механизма
    m_hybridProtocol->start();   // ← главный запуск

    m_isConnectionInProgress = true;
    m_connectionStateText = tr("Connecting...");
    emit connectionStateChanged();
}

void ConnectionController::closeConnection()
{
    if (m_hybridProtocol) {
        m_hybridProtocol->stop();
    }
    emit disconnectFromVpn(); // оставляем для совместимости со старым VpnConnection
}

ErrorCode ConnectionController::getLastConnectionError()
{
    return m_vpnConnection->lastError();
}

void ConnectionController::onConnectionStateChanged(Vpn::ConnectionState state)
{
    // Оставляем для совместимости, но Hybrid управляет состоянием сам
    m_state = state;
    emit connectionStateChanged();
}

void ConnectionController::onCurrentContainerUpdated()
{
    if (m_isConnected || m_isConnectionInProgress) {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully, reconnection..."));
        closeConnection();
        QTimer::singleShot(1000, this, &ConnectionController::openConnection);
    } else {
        emit reconnectWithUpdatedContainer(tr("Settings updated successfully"));
    }
}

void ConnectionController::onTranslationsUpdated()
{
    onConnectionStateChanged(getCurrentConnectionState());
}

Vpn::ConnectionState ConnectionController::getCurrentConnectionState()
{
    return m_state;
}

QString ConnectionController::connectionStateText() const
{
    return m_connectionStateText;
}

void ConnectionController::toggleConnection()
{
    if (m_state == Vpn::ConnectionState::Preparing) {
        emit preparingConfig();
        return;
    }

    if (isConnectionInProgress() || isConnected()) {
        closeConnection();
    } else {
        emit prepareConfig();
        openConnection();   // сразу запускаем Hybrid
    }
}

bool ConnectionController::isConnectionInProgress() const
{
    return m_isConnectionInProgress;
}

bool ConnectionController::isConnected() const
{
    return m_isConnected;
}