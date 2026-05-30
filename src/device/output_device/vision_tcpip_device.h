#ifndef VISION_TCPIP_DEVICE_H
#define VISION_TCPIP_DEVICE_H

#include "device/output_device/vision_output_device.h"
#include "device/output_device/vision_tcpip_config.h"
#include "device/output_device/vision_output_request.h"

#include <QByteArray>
#include <QElapsedTimer>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>

namespace vc::device {

#define VISION_OUTPUT_HB_MESSAGE        "connection_check."
#define VISION_OUTPUT_HB_ACK_PREFIX     "ack,"
#define VISION_OUTPUT_HB_TERMINATOR     '.'
#define VISION_OUTPUT_MSG_COUNT_LIMIT   (1 << 16)

/**
 * @brief VisionTcpipDevice
 *
 *  TCP/IP server với 2 port theo mô tả trong
 *  docs/Tcpip_OutputDevice_description.md.
 *
 *  - Port chính (main): nhận yêu cầu matching và đẩy kết quả về client.
 *  - Port heartbeat: server gửi "connection_check." mỗi
 *    `heartbeatIntervalMs`, mong nhận "ack,{msg_count}." từ client.
 *    msg_count đếm tăng từ 0 và reset khi vượt 2^16.
 *
 *  Khi client không phản hồi đúng / kịp -> device chuyển sang
 *  LostConnected và đóng cả 2 kết nối hiện tại (nhưng vẫn lắng nghe
 *  cho lần kết nối lại).
 */
class VisionTcpipDevice : public VisionOutputDevice {
    Q_OBJECT

public:
    explicit VisionTcpipDevice(QString id, QString name, QObject* parent = nullptr);
    ~VisionTcpipDevice() override;

    VisionOutputType visionOutputType() const override {
        return VisionOutputType::VisionTCPIP;
    }

    bool deviceConnect() override;
    bool deviceDisconnect() override;
    bool isDeviceConnected() const override {
        return connectStatus() == ConnectStatus::Connected;
    }

    void setDeviceConfig(IDeviceCfg *cfg) override;
    void setVisionTcpipConfig(VisionTcpipDeviceCfg& cfg);
    VisionTcpipDeviceCfg visionTcpipConfig() const { return m_config; }

    /**
     * @brief Cập nhật và đẩy ngay 1 request kết quả matching xuống main port.
     *        Trả false nếu chưa có client kết nối hoặc request sai loại.
     */
    bool pushRequest(IRequest *request) override;

    QStringList getAvailableBits() override   { return QStringList(); }
    QStringList getAvailableWords() override  { return QStringList(); }

    bool fromJson(const QJsonObject &obj) override;

    // ==== Trạng thái runtime cho test / UI ====
    bool isMainClientConnected() const      { return m_mainClient != nullptr; }
    bool isHeartbeatClientConnected() const { return m_hbClient   != nullptr; }
    quint16 lastAckCount() const            { return m_lastAckCount; }
    quint16 expectedAckCount() const        { return m_expectedAckCount; }

public slots:
    void deviceTerminate() override;

signals:
    /// Phát khi nhận đầy đủ 1 message từ port chính (kết thúc bằng ';').
    void mainRequestReceived(const QByteArray &payload);
    /// Phát mỗi khi gửi xong 1 kết quả xuống main port.
    void resultSent(const QByteArray &payload);
    /// Phát khi heartbeat phát hiện lost connect (timeout/sai format).
    void heartbeatLost(const QString &reason);

private slots:
    void onMainNewConnection();
    void onMainClientDisconnected();
    void onMainReadyRead();

    void onHeartbeatNewConnection();
    void onHeartbeatClientDisconnected();
    void onHeartbeatReadyRead();
    void onHeartbeatTick();

private:
    bool startServers();
    void stopServers();

    void handleMainPayload(const QByteArray &payload);
    void handleHeartbeatPayload(const QByteArray &payload);

    void sendHeartbeatProbe();
    void declareLostConnection(const QString &reason);

    void resetHeartbeatState();

private:
    VisionTcpipDeviceCfg m_config;

    QTcpServer *m_mainServer{nullptr};
    QTcpServer *m_hbServer{nullptr};

    QTcpSocket *m_mainClient{nullptr};
    QTcpSocket *m_hbClient{nullptr};

    QByteArray m_mainRxBuffer;
    QByteArray m_hbRxBuffer;

    QTimer *m_hbTimer{nullptr};
    QElapsedTimer m_hbLastReplyTimer;

    quint16 m_expectedAckCount{0};
    quint16 m_lastAckCount{0};
    bool m_hbAwaitingReply{false};
};

} // namespace vc::device

#endif // VISION_TCPIP_DEVICE_H
