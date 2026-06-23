#ifndef MC_PROTOCOL_DEVICE_H
#define MC_PROTOCOL_DEVICE_H

#include "device/plc/plc_device.h"
#include "device/plc/mc_protocol_config.h"
#include "device/plc/mc_fame_3e.h"
#include "device/device_capabilities.h"

#include <QTimer>
#include <chrono>

#define MC_PROTOCOL_DEVICE_STR      "MC protocol device"

namespace vc::device {

class McProtocolDevice : public PlcDevice, public IPlcTagProvider, public IPlcIoWriter {
    Q_OBJECT

public:
    explicit McProtocolDevice(QString id, QString name, QObject* parent = nullptr);
    ~McProtocolDevice();

    bool deviceConnect() override;
    bool deviceDisconnect() override;
    bool isDeviceConnected() const override {
        return connectStatus() == device::ConnectStatus::Connected;
    }

    PlcType plcType() const override {
        return PlcType::MitsubishiMc;
    }

    McFrameType currentFrameType() const {
        return m_config.currentFrameType();
    }

    void setDeviceConfig(IDeviceCfg *cfg) override;
    void setMcProtocolConfig(McProtocolConfig& cfg);
    McProtocolConfig mcProtocolConfig() const;

    QStringList availableDigitalIoNames() const override;
    QStringList availableWordIoNames() const override;
    bool writeDigitalIoByName(const QString &tag, bool value) override;
    bool writeWordIoByName(const QString &tag, qint16 value) override;

    bool pushRequest(IRequest *request) override;

    bool fromJson(const QJsonObject &obj) override;

public slots:
    void deviceTerminate() override;

private slots:
    void onPollingTimerTimeOut();
    void onMsgInterfaceReadReady();
    void onSetCommActiveDevice();

private:
    enum DataQueryState {
        QueryTriggerByTimer,
        QueryContinue,
        QueryFinished
    };

    bool initialize_mc_device();
    // Free the transport + frame and stop (but keep) the polling timer.
    // Caller must hold m_mutex. Does not change connection status.
    void releaseConnectionResources();
    // Release transport resources and publish the given terminal status.
    // Locks m_mutex; safe to call from the polling/response handlers.
    void teardownConnection(ConnectStatus finalStatus);
    void excute_request();
    void polling_query();

    void request_handle();
    void response_handle();
    void retry_request_handle();

    void optimizeDeviceMap();
    void update_m_map();
    void update_d_map();
    void check_device_changed();
    void update_last_m_map();
    void update_last_d_map();

    void setDeviceLostConnect();

signals:
    void requestFinished(vc::device::McResult result);
    void deviceMChanged(int number, quint8 last_state, quint8 new_state);
    void deviceDChanged(int number, qint16 last_val, qint16 new_val);

private:
    McProtocolConfig m_config;

    std::unique_ptr<McMsgInterface> m_msg_interface;
    std::unique_ptr<MCFrameAbstract> m_frame;

    QList<std::shared_ptr<MCRequest>> m_request_queue;
    QList<std::shared_ptr<MCRequest>> m_polling_request_queue;

    std::shared_ptr<MCRequest> m_current_request;

    McDeviceMap m_device_map;
    QTimer *m_polling_timer{nullptr};

    std::chrono::high_resolution_clock::time_point m_sent_time_point;

    bool m_comm_active_m_device_value{false};
    int m_comm_active_m_device;
    DataQueryState m_data_update_state;
    int m_update_command_index;
    bool is_first_time_polling;
    bool m_wait_for_response;
    // bool m_retry_by_timeout{false};
    int m_retry_count{0};

    std::map<int, quint8> m_last_device_M_map;
    std::map<int, qint16> m_last_device_D_map;

    QStringList m_m_device_names;
    QStringList m_d_device_names;

    QByteArray m_read_buffer;
};


} // namespace vc::device

#endif // MC_PROTOCOL_DEVICE_H
