#ifndef VISION_TCPIP_DEVICE_H
#define VISION_TCPIP_DEVICE_H

#include "device/output_device/vision_tcpip_device_base.h"
#include "device/output_device/vision_tcpip_config.h"

#include <QTcpServer>

namespace vc::device {

/**
 * @brief VisionTcpipDevice — TCP/IP **server** transport of the vision-output
 *        family.
 *
 *  Listens on two ports (main + heartbeat), accepts at most one client per
 *  port (pending duplicates are rejected), and delegates all protocol
 *  handling to VisionTcpipDeviceBase. The software remains the heartbeat
 *  master: it sends "connection_check." and expects "ack,{count}.".
 *
 *  On lost connection the device drops the current client sockets but keeps
 *  the listeners open for the next reconnect.
 */
class VisionTcpipDevice : public VisionTcpipDeviceBase {
    Q_OBJECT

public:
    explicit VisionTcpipDevice(QString id, QString name, QObject* parent = nullptr);
    ~VisionTcpipDevice() override;

    VisionOutputType visionOutputType() const override {
        return VisionOutputType::VisionTCPIP;
    }

    void setDeviceConfig(IDeviceCfg *cfg) override;
    // Returns false (config unchanged) when the device is connected — the
    // config is locked while the link is live. Caller should surface this.
    bool setVisionTcpipConfig(VisionTcpipDeviceCfg& cfg);
    VisionTcpipDeviceCfg visionTcpipConfig() const { return m_config; }

    bool fromJson(const QJsonObject &obj) override;

protected:
    // ── Transport: listen + accept ─────────────────────────────────────────
    bool startTransport() override;
    void stopTransport() override;

    int cfgMainPort() const override            { return m_config.m_mainPort; }
    int cfgHeartbeatPort() const override        { return m_config.m_heartbeatPort; }
    int cfgHeartbeatIntervalMs() const override  { return m_config.m_heartbeatIntervalMs; }
    int cfgHeartbeatTimeoutMs() const override   { return m_config.m_heartbeatTimeoutMs; }
    RobotKinematicCheckConfig kinematicCheckConfig() const override { return m_config.m_kinematicCheck; }

private slots:
    void onMainNewConnection();
    void onHeartbeatNewConnection();

private:
    VisionTcpipDeviceCfg m_config;
    QTcpServer *m_mainServer{nullptr};
    QTcpServer *m_hbServer{nullptr};
};

} // namespace vc::device

#endif // VISION_TCPIP_DEVICE_H
