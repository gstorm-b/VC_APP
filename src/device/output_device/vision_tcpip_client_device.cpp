#include "vision_tcpip_client_device.h"

#include <QMutexLocker>
#include <utility>

namespace vc::device {

VisionTcpipClientDevice::VisionTcpipClientDevice(QString id, QString name, QObject* parent)
    : VisionTcpipDeviceBase(std::move(id), std::move(name), parent) {

    IDevice::setDeviceConfig(&m_config);
    syncRuntimeState();
}

VisionTcpipClientDevice::~VisionTcpipClientDevice() {
    stopTransport();
}

void VisionTcpipClientDevice::setDeviceConfig(IDeviceCfg *cfg) {
    if (!cfg) {
        return;
    }
    if (cfg->deviceType() != DeviceType::VisionOutput) {
        LOG_DEV_ERR << "VisionTcpipClientDevice: wrong config type";
        return;
    }
    auto *vcfg = dynamic_cast<VisionOutputDeviceCfg*>(cfg);
    if (!vcfg || vcfg->visionOutputType() != VisionOutputType::VisionTcpipClient) {
        LOG_DEV_ERR << "VisionTcpipClientDevice: wrong sub-type config";
        return;
    }
    if (isDeviceConnected()) {
        LOG_DEV_INFO << "VisionTcpipClientDevice: cannot change config while connected";
        return;
    }

    QMutexLocker locker(&m_mutex);
    VisionTcpipClientDeviceCfg *casted = static_cast<VisionTcpipClientDeviceCfg*>(cfg);
    m_config = *casted;
    IDevice::setDeviceConfig(&m_config);
}

void VisionTcpipClientDevice::setVisionTcpipClientConfig(VisionTcpipClientDeviceCfg& cfg) {
    if (this->isDeviceConnected()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_config = cfg;
    IDevice::setDeviceConfig(&m_config);
}

bool VisionTcpipClientDevice::fromJson(const QJsonObject &obj) {
    QMutexLocker locker(&m_mutex);
    return IDevice::fromJson(obj);
}

// =====================================================================
// Transport: dial out. Connected is reported ONLY once both the main and
// heartbeat links are actually up (see evaluateConnected()); while dialing
// or reconnecting the status is Connecting.
// =====================================================================
bool VisionTcpipClientDevice::startTransport() {
    setConnectionStatus(ConnectStatus::Connecting);
    attemptConnect();
    LOG_DEV_INFO << "VisionTcpipClientDevice dialing" << m_config.m_serverAddress
                 << "main port:" << m_config.m_mainPort
                 << "heartbeat port:" << m_config.m_heartbeatPort;
    return true;
}

void VisionTcpipClientDevice::stopTransport() {
    if (m_reconnectTimer && m_reconnectTimer->isActive()) {
        m_reconnectTimer->stop();
    }

    if (m_mainConnector) {
        m_mainConnector->disconnect(this);
        m_mainConnector->abort();
        m_mainConnector->deleteLater();
        m_mainConnector = nullptr;
    }
    if (m_hbConnector) {
        m_hbConnector->disconnect(this);
        m_hbConnector->abort();
        m_hbConnector->deleteLater();
        m_hbConnector = nullptr;
    }

    detachHeartbeatSocket();
    detachMainSocket();
}

void VisionTcpipClientDevice::onLinkLost() {
    // A link dropped. If we were fully Connected, fall back to Connecting; a
    // heartbeat-loss path already set LostConnected and must stay visible
    // until the redial actually starts (handled in attemptConnect()).
    if (connectStatus() == ConnectStatus::Connected) {
        setConnectionStatus(ConnectStatus::Connecting);
    }
    scheduleReconnect();
}

// =====================================================================
// Outbound connection management
// =====================================================================
void VisionTcpipClientDevice::attemptConnect() {
    if (!isActive()) {
        return;
    }

    // Redialing (e.g. after LostConnected): reflect the in-progress state.
    if (connectStatus() != ConnectStatus::Connected
        && connectStatus() != ConnectStatus::Connecting) {
        setConnectionStatus(ConnectStatus::Connecting);
    }

    if (!m_mainSocket && !m_mainConnector) {
        m_mainConnector = new QTcpSocket(this);
        connect(m_mainConnector, &QTcpSocket::connected,
                this, &VisionTcpipClientDevice::onMainConnected);
        connect(m_mainConnector, &QAbstractSocket::errorOccurred,
                this, &VisionTcpipClientDevice::onMainConnectError);
        m_mainConnector->connectToHost(m_config.m_serverAddress,
                                       static_cast<quint16>(m_config.m_mainPort));
    }

    if (!m_hbSocket && !m_hbConnector) {
        m_hbConnector = new QTcpSocket(this);
        connect(m_hbConnector, &QTcpSocket::connected,
                this, &VisionTcpipClientDevice::onHeartbeatConnected);
        connect(m_hbConnector, &QAbstractSocket::errorOccurred,
                this, &VisionTcpipClientDevice::onHeartbeatConnectError);
        m_hbConnector->connectToHost(m_config.m_serverAddress,
                                     static_cast<quint16>(m_config.m_heartbeatPort));
    }
}

void VisionTcpipClientDevice::onMainConnected() {
    QTcpSocket *sock = m_mainConnector;
    m_mainConnector = nullptr;
    if (!sock) return;

    // Drop the dialing-stage handlers; the base re-wires readyRead/disconnected.
    sock->disconnect(this);
    attachMainSocket(sock);
    evaluateConnected();
}

void VisionTcpipClientDevice::onHeartbeatConnected() {
    QTcpSocket *sock = m_hbConnector;
    m_hbConnector = nullptr;
    if (!sock) return;

    sock->disconnect(this);
    attachHeartbeatSocket(sock);
    evaluateConnected();
}

void VisionTcpipClientDevice::onMainConnectError(QAbstractSocket::SocketError err) {
    Q_UNUSED(err);
    if (m_mainConnector) {
        m_diagnostics.lastError = QString("Main connect failed: %1")
                                      .arg(m_mainConnector->errorString());
        m_mainConnector->disconnect(this);
        m_mainConnector->deleteLater();
        m_mainConnector = nullptr;
    }
    if (isActive()) {
        scheduleReconnect();
    }
}

void VisionTcpipClientDevice::onHeartbeatConnectError(QAbstractSocket::SocketError err) {
    Q_UNUSED(err);
    if (m_hbConnector) {
        m_diagnostics.lastError = QString("Heartbeat connect failed: %1")
                                      .arg(m_hbConnector->errorString());
        m_hbConnector->disconnect(this);
        m_hbConnector->deleteLater();
        m_hbConnector = nullptr;
    }
    if (isActive()) {
        scheduleReconnect();
    }
}

void VisionTcpipClientDevice::scheduleReconnect() {
    if (!isActive()) {
        return;
    }
    if (!m_reconnectTimer) {
        m_reconnectTimer = new QTimer(this);
        m_reconnectTimer->setSingleShot(true);
        connect(m_reconnectTimer, &QTimer::timeout,
                this, &VisionTcpipClientDevice::attemptConnect);
    }
    if (!m_reconnectTimer->isActive()) {
        m_reconnectTimer->start(m_config.m_reconnectIntervalMs);
    }
}

void VisionTcpipClientDevice::evaluateConnected() {
    // Connected is reported ONLY when both the main and heartbeat links are up.
    if (isActive() && m_mainSocket && m_hbSocket
        && m_mainSocket->state() == QAbstractSocket::ConnectedState
        && m_hbSocket->state() == QAbstractSocket::ConnectedState) {
        if (connectStatus() != ConnectStatus::Connected) {
            setConnectionStatus(ConnectStatus::Connected);
        }
    }
}

} // namespace vc::device
