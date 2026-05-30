#include "vision_tcpip_device.h"

#include <QHostAddress>
#include <QMutexLocker>

namespace vc::device {

VisionTcpipDevice::VisionTcpipDevice(QString id, QString name, QObject* parent)
    : VisionOutputDevice(id, name, parent) {

    IDevice::setDeviceConfig(&m_config);
}

VisionTcpipDevice::~VisionTcpipDevice() {
    stopServers();
}

void VisionTcpipDevice::deviceTerminate() {
    LOG_DEV_DEBUG << "VisionTcpipDevice terminate" << name() << "id" << id();
    if (isDeviceConnected() || m_mainServer || m_hbServer) {
        deviceDisconnect();
    }
}

bool VisionTcpipDevice::deviceConnect() {
    QMutexLocker locker(&m_mutex);

    if (m_mainServer || m_hbServer) {
        LOG_DEV_INFO << "VisionTcpipDevice already started";
        return true;
    }

    if (!startServers()) {
        stopServers();
        setConnectionStatus(ConnectStatus::ConnectFailed, m_last_msg);
        return false;
    }

    setConnectionStatus(ConnectStatus::Connected);
    LOG_DEV_INFO << "VisionTcpipDevice listening, main port:" << m_config.m_mainPort
                 << " heartbeat port:" << m_config.m_heartbeatPort;
    return true;
}

bool VisionTcpipDevice::deviceDisconnect() {
    QMutexLocker locker(&m_mutex);
    stopServers();
    setConnectionStatus(ConnectStatus::Disconnected);
    LOG_DEV_INFO << "VisionTcpipDevice disconnected";
    return true;
}

void VisionTcpipDevice::setDeviceConfig(IDeviceCfg *cfg) {
    if (!cfg) {
        return;
    }
    if (cfg->deviceType() != DeviceType::VisionOutput) {
        LOG_DEV_ERR << "VisionTcpipDevice: wrong config type";
        return;
    }
    // Reject configs whose VisionOutputType is anything other than VisionTCPIP.
    auto *vcfg = dynamic_cast<VisionOutputDeviceCfg*>(cfg);
    if (!vcfg || vcfg->visionOutputType() != VisionOutputType::VisionTCPIP) {
        LOG_DEV_ERR << "VisionTcpipDevice: wrong sub-type config";
        return;
    }
    if (isDeviceConnected()) {
        LOG_DEV_INFO << "VisionTcpipDevice: cannot change config while connected";
        return;
    }

    QMutexLocker locker(&m_mutex);
    VisionTcpipDeviceCfg *casted = static_cast<VisionTcpipDeviceCfg*>(cfg);
    m_config = *casted;
    IDevice::setDeviceConfig(&m_config);
}

void VisionTcpipDevice::setVisionTcpipConfig(VisionTcpipDeviceCfg& cfg) {
    if (this->isDeviceConnected()) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    m_config = cfg;
    // change and emit signal
    IDevice::setDeviceConfig(&m_config);
}

bool VisionTcpipDevice::pushRequest(IRequest *request) {
    if (!request || request->type() != RequestType::Request_VisionOutput) {
        return false;
    }
    if (!m_mainClient || m_mainClient->state() != QAbstractSocket::ConnectedState) {
        LOG_DEV_INFO << "VisionTcpipDevice: drop request, no main client connected";
        return false;
    }

    VisionOutputRequest *vreq = static_cast<VisionOutputRequest*>(request);
    QByteArray payload = vreq->buildPayload();

    qint64 written = m_mainClient->write(payload);
    if (written != payload.size()) {
        LOG_DEV_ERR << "VisionTcpipDevice: short write" << written << "/" << payload.size();
        return false;
    }
    m_mainClient->flush();

    emit resultSent(payload);
    return true;
}

bool VisionTcpipDevice::fromJson(const QJsonObject &obj) {
    QMutexLocker locker(&m_mutex);
    return IDevice::fromJson(obj);
}

// =====================================================================
// Server lifecycle
// =====================================================================
bool VisionTcpipDevice::startServers() {
    m_mainServer = new QTcpServer(this);
    m_hbServer   = new QTcpServer(this);

    connect(m_mainServer, &QTcpServer::newConnection,
            this, &VisionTcpipDevice::onMainNewConnection);
    connect(m_hbServer, &QTcpServer::newConnection,
            this, &VisionTcpipDevice::onHeartbeatNewConnection);

    QHostAddress addr(m_config.m_listenAddress);
    if (addr.isNull()) {
        addr = QHostAddress::Any;
    }

    if (!m_mainServer->listen(addr, static_cast<quint16>(m_config.m_mainPort))) {
        m_last_msg = QString("Cannot listen on main port %1: %2")
                         .arg(m_config.m_mainPort)
                         .arg(m_mainServer->errorString());
        LOG_DEV_ERR << m_last_msg;
        return false;
    }
    if (!m_hbServer->listen(addr, static_cast<quint16>(m_config.m_heartbeatPort))) {
        m_last_msg = QString("Cannot listen on heartbeat port %1: %2")
                         .arg(m_config.m_heartbeatPort)
                         .arg(m_hbServer->errorString());
        LOG_DEV_ERR << m_last_msg;
        return false;
    }

    if (!m_hbTimer) {
        m_hbTimer = new QTimer(this);
        m_hbTimer->setTimerType(Qt::PreciseTimer);
        connect(m_hbTimer, &QTimer::timeout,
                this, &VisionTcpipDevice::onHeartbeatTick);
    }
    m_hbTimer->setInterval(m_config.m_heartbeatIntervalMs);

    return true;
}

void VisionTcpipDevice::stopServers() {
    if (m_hbTimer) {
        if (m_hbTimer->isActive()) m_hbTimer->stop();
        m_hbTimer->deleteLater();
        m_hbTimer = nullptr;
    }

    if (m_mainClient) {
        m_mainClient->disconnect(this);
        m_mainClient->abort();
        m_mainClient->deleteLater();
        m_mainClient = nullptr;
    }
    if (m_hbClient) {
        m_hbClient->disconnect(this);
        m_hbClient->abort();
        m_hbClient->deleteLater();
        m_hbClient = nullptr;
    }
    if (m_mainServer) {
        m_mainServer->close();
        m_mainServer->deleteLater();
        m_mainServer = nullptr;
    }
    if (m_hbServer) {
        m_hbServer->close();
        m_hbServer->deleteLater();
        m_hbServer = nullptr;
    }

    m_mainRxBuffer.clear();
    m_hbRxBuffer.clear();
    resetHeartbeatState();
}

void VisionTcpipDevice::resetHeartbeatState() {
    m_expectedAckCount = 0;
    m_lastAckCount     = 0;
    m_hbAwaitingReply  = false;
    m_hbLastReplyTimer.invalidate();
}

// =====================================================================
// Main port
// =====================================================================
void VisionTcpipDevice::onMainNewConnection() {
    while (m_mainServer && m_mainServer->hasPendingConnections()) {
        QTcpSocket *sock = m_mainServer->nextPendingConnection();
        if (m_mainClient) {
            LOG_DEV_INFO << "VisionTcpipDevice: main port already in use, reject"
                         << sock->peerAddress().toString();
            sock->disconnectFromHost();
            sock->deleteLater();
            continue;
        }

        m_mainClient = sock;
        m_mainRxBuffer.clear();
        connect(m_mainClient, &QTcpSocket::disconnected,
                this, &VisionTcpipDevice::onMainClientDisconnected);
        connect(m_mainClient, &QTcpSocket::readyRead,
                this, &VisionTcpipDevice::onMainReadyRead);

        LOG_DEV_INFO << "VisionTcpipDevice main client connected from"
                     << m_mainClient->peerAddress().toString();
    }
}

void VisionTcpipDevice::onMainClientDisconnected() {
    if (!m_mainClient) return;
    LOG_DEV_INFO << "VisionTcpipDevice main client disconnected";
    m_mainClient->deleteLater();
    m_mainClient = nullptr;
    m_mainRxBuffer.clear();
}

void VisionTcpipDevice::onMainReadyRead() {
    if (!m_mainClient) return;
    m_mainRxBuffer.append(m_mainClient->readAll());

    // Mỗi message kết thúc bởi ';' theo spec format kết quả.
    int idx = m_mainRxBuffer.indexOf(';');
    while (idx >= 0) {
        QByteArray msg = m_mainRxBuffer.left(idx + 1);
        m_mainRxBuffer.remove(0, idx + 1);
        handleMainPayload(msg);
        idx = m_mainRxBuffer.indexOf(';');
    }
}

void VisionTcpipDevice::handleMainPayload(const QByteArray &payload) {
    LOG_DEV_DEBUG << "VisionTcpipDevice main RX:" << payload;
    emit mainRequestReceived(payload);
}

// =====================================================================
// Heartbeat port
// =====================================================================
void VisionTcpipDevice::onHeartbeatNewConnection() {
    while (m_hbServer && m_hbServer->hasPendingConnections()) {
        QTcpSocket *sock = m_hbServer->nextPendingConnection();
        if (m_hbClient) {
            LOG_DEV_INFO << "VisionTcpipDevice: heartbeat port already in use, reject"
                         << sock->peerAddress().toString();
            sock->disconnectFromHost();
            sock->deleteLater();
            continue;
        }

        m_hbClient = sock;
        m_hbRxBuffer.clear();
        resetHeartbeatState();

        connect(m_hbClient, &QTcpSocket::disconnected,
                this, &VisionTcpipDevice::onHeartbeatClientDisconnected);
        connect(m_hbClient, &QTcpSocket::readyRead,
                this, &VisionTcpipDevice::onHeartbeatReadyRead);

        LOG_DEV_INFO << "VisionTcpipDevice heartbeat client connected from"
                     << m_hbClient->peerAddress().toString();

        // Theo spec: "Sau khi connect Port 2 sẽ gửi một package và chờ client reply".
        sendHeartbeatProbe();
        if (m_hbTimer) {
            m_hbTimer->start();
        }
    }
}

void VisionTcpipDevice::onHeartbeatClientDisconnected() {
    if (!m_hbClient) return;
    LOG_DEV_INFO << "VisionTcpipDevice heartbeat client disconnected";
    m_hbClient->deleteLater();
    m_hbClient = nullptr;
    m_hbRxBuffer.clear();
    if (m_hbTimer && m_hbTimer->isActive()) {
        m_hbTimer->stop();
    }
    resetHeartbeatState();
}

void VisionTcpipDevice::onHeartbeatReadyRead() {
    if (!m_hbClient) return;
    m_hbRxBuffer.append(m_hbClient->readAll());

    int idx = m_hbRxBuffer.indexOf(VISION_OUTPUT_HB_TERMINATOR);
    while (idx >= 0) {
        QByteArray msg = m_hbRxBuffer.left(idx + 1);
        m_hbRxBuffer.remove(0, idx + 1);
        handleHeartbeatPayload(msg);
        idx = m_hbRxBuffer.indexOf(VISION_OUTPUT_HB_TERMINATOR);
    }
}

void VisionTcpipDevice::handleHeartbeatPayload(const QByteArray &payload) {
    // Format hợp lệ: "ack,{msg_count}."
    if (!payload.startsWith(VISION_OUTPUT_HB_ACK_PREFIX) || !payload.endsWith('.')) {
        declareLostConnection(QString("Invalid heartbeat reply format: %1")
                                  .arg(QString::fromUtf8(payload)));
        return;
    }

    QByteArray middle = payload.mid(qstrlen(VISION_OUTPUT_HB_ACK_PREFIX),
                                    payload.size() - qstrlen(VISION_OUTPUT_HB_ACK_PREFIX) - 1);
    bool ok = false;
    int parsed = middle.toInt(&ok);
    if (!ok || parsed < 0 || parsed >= VISION_OUTPUT_MSG_COUNT_LIMIT) {
        declareLostConnection(QString("Invalid heartbeat msg_count: %1")
                                  .arg(QString::fromUtf8(middle)));
        return;
    }

    if (static_cast<quint16>(parsed) != m_expectedAckCount) {
        declareLostConnection(QString("Heartbeat msg_count mismatch, expected %1 got %2")
                                  .arg(m_expectedAckCount).arg(parsed));
        return;
    }

    m_lastAckCount     = static_cast<quint16>(parsed);
    m_hbAwaitingReply  = false;
    m_hbLastReplyTimer.restart();

    // Tăng counter, reset khi vượt limit (2^16).
    m_expectedAckCount = static_cast<quint16>((m_expectedAckCount + 1) % VISION_OUTPUT_MSG_COUNT_LIMIT);
}

void VisionTcpipDevice::onHeartbeatTick() {
    if (!m_hbClient || m_hbClient->state() != QAbstractSocket::ConnectedState) {
        return;
    }

    // Nếu đang chờ reply mà quá timeout thì lost connect.
    if (m_hbAwaitingReply && m_hbLastReplyTimer.isValid()
        && m_hbLastReplyTimer.elapsed() > m_config.m_heartbeatTimeoutMs) {
        declareLostConnection(QString("Heartbeat reply timeout (%1 ms)")
                                  .arg(m_config.m_heartbeatTimeoutMs));
        return;
    }

    sendHeartbeatProbe();
}

void VisionTcpipDevice::sendHeartbeatProbe() {
    if (!m_hbClient) return;

    QByteArray probe(VISION_OUTPUT_HB_MESSAGE);
    m_hbClient->write(probe);
    m_hbClient->flush();

    m_hbAwaitingReply = true;
    // Timer đo "thời gian từ lần reply hợp lệ gần nhất" (hoặc từ probe đầu
    // tiên nếu chưa có reply). Không restart ở đây — nếu restart mỗi probe
    // thì timeout sẽ không bao giờ trigger.
    if (!m_hbLastReplyTimer.isValid()) {
        m_hbLastReplyTimer.start();
    }
}

void VisionTcpipDevice::declareLostConnection(const QString &reason) {
    LOG_DEV_ERR << "VisionTcpipDevice lost connection:" << reason;
    emit heartbeatLost(reason);

    if (m_hbTimer && m_hbTimer->isActive()) {
        m_hbTimer->stop();
    }
    if (m_hbClient) {
        m_hbClient->disconnect(this);
        m_hbClient->abort();
        m_hbClient->deleteLater();
        m_hbClient = nullptr;
    }
    if (m_mainClient) {
        m_mainClient->disconnect(this);
        m_mainClient->abort();
        m_mainClient->deleteLater();
        m_mainClient = nullptr;
    }
    m_mainRxBuffer.clear();
    m_hbRxBuffer.clear();
    resetHeartbeatState();

    setConnectionStatus(ConnectStatus::LostConnected, reason);
}

} // namespace vc::device
