#ifndef VISION_TCPIP_CLIENT_DEVICE_H
#define VISION_TCPIP_CLIENT_DEVICE_H

#include "device/output_device/vision_tcpip_device_base.h"
#include "device/output_device/vision_tcpip_client_config.h"

namespace vc::device {

/**
 * @brief VisionTcpipClientDevice — TCP/IP **client** transport of the
 *        vision-output family.
 *
 *  Same protocol as VisionTcpipDevice, but the software dials OUT to a remote
 *  endpoint (which acts as the TCP server, e.g. a robot controller). It keeps
 *  the same data/heartbeat semantics: the software is still the heartbeat
 *  master ("connection_check." -> "ack,{count}.") and still pushes matching
 *  results out on the main channel. Only the connection direction differs:
 *  QTcpSocket::connectToHost + auto-reconnect instead of QTcpServer::listen.
 *
 *  When a link drops or the heartbeat times out, the device retries the
 *  outbound connection every `reconnectIntervalMs` while active.
 */
class VisionTcpipClientDevice : public VisionTcpipDeviceBase {
    Q_OBJECT

public:
    explicit VisionTcpipClientDevice(QString id, QString name, QObject* parent = nullptr);
    ~VisionTcpipClientDevice() override;

    VisionOutputType visionOutputType() const override {
        return VisionOutputType::VisionTcpipClient;
    }

    void setDeviceConfig(IDeviceCfg *cfg) override;
    void setVisionTcpipClientConfig(VisionTcpipClientDeviceCfg& cfg);
    VisionTcpipClientDeviceCfg visionTcpipClientConfig() const { return m_config; }

    bool fromJson(const QJsonObject &obj) override;

protected:
    // ── Transport: dial out + reconnect ────────────────────────────────────
    bool startTransport() override;
    void stopTransport() override;
    void onLinkLost() override;

    int cfgMainPort() const override            { return m_config.m_mainPort; }
    int cfgHeartbeatPort() const override        { return m_config.m_heartbeatPort; }
    int cfgHeartbeatIntervalMs() const override  { return m_config.m_heartbeatIntervalMs; }
    int cfgHeartbeatTimeoutMs() const override   { return m_config.m_heartbeatTimeoutMs; }
    RobotKinematicCheckConfig kinematicCheckConfig() const override { return m_config.m_kinematicCheck; }

private slots:
    void attemptConnect();
    void onMainConnected();
    void onHeartbeatConnected();
    void onMainConnectError(QAbstractSocket::SocketError err);
    void onHeartbeatConnectError(QAbstractSocket::SocketError err);

private:
    void scheduleReconnect();
    void evaluateConnected();

    VisionTcpipClientDeviceCfg m_config;

    // Sockets while still dialing (before the link comes up). Once connected
    // they are handed to the base as m_mainSocket / m_hbSocket.
    QTcpSocket *m_mainConnector{nullptr};
    QTcpSocket *m_hbConnector{nullptr};

    QTimer *m_reconnectTimer{nullptr};
};

} // namespace vc::device

#endif // VISION_TCPIP_CLIENT_DEVICE_H
