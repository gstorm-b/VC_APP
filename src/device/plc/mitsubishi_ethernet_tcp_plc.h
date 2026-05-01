#ifndef MITSUBISHI_ETHERNET_TCP_PLC_H
#define MITSUBISHI_ETHERNET_TCP_PLC_H

// #include "device/plc/mc_frame_abstract.h"
// #include "device/plc/mc_protocol_device.h"
// #include "device/plc/mc_msg_tcp_client.h"
// #include "device/plc/mc_fame_3e.h"
// #include <QTimer>

// namespace vc::device {

// enum McFrame{
//     Frame_1C,
//     Frame_3C,
//     Frame_1E,
//     Frame_3E
// };

// enum McDataCode {
//     DataBinary,
//     DataAscii
// };

// class McItfParams {

// };

// class MCCfg : public PlcCfg {
// public:
//     explicit MCCfg()
//         : PlcCfg() {}

//     virtual McFrame frameType() const = 0;
//     virtual McDataCode dataCode() const = 0;

//     virtual std::shared_ptr<MCCfg> clone() const = 0;

//     virtual QJsonObject toJson() const override;
//     virtual bool fromJson(const QJsonObject &obj) override;

// public:
//     McDataCode data_code{McDataCode::DataBinary};
//     int refresh_interval{100};
//     int start_m_address{2000};
//     int amount_m_devices{64};
//     int start_d_address{2000};
//     int amount_d_devices{64};
// };

// class MC3ECfg : public MCCfg {
// public:
//     explicit MC3ECfg()
//         : MCCfg() {}

//     McFrame frameType() const override {
//         return frame_type;
//     }

//     McDataCode dataCode() const override {
//         return data_code;
//     }

//     std::shared_ptr<MCCfg> clone() const override {
//         return std::make_shared<MC3ECfg>(*this);
//     }

//     QJsonObject toJson() const override;
//     bool fromJson(const QJsonObject &obj) override;

// public:
//     QString ip_address;
//     int port_number;
//     const McFrame frame_type = McFrame::Frame_3E;
//     Context_Mc3E frame_context;
// };

// class MCEthernetTCPPlc : public PLCDevice {
//     Q_OBJECT

// public:
//     explicit MCEthernetTCPPlc(QString id, QString name, QObject* parent = nullptr);

//     bool plcInitialize() override;
//     void plcTerminate() override;

//     bool deviceConnect() override;
//     bool deviceDisconnect() override;
//     bool isDeviceConnected() const override {
//         return m_is_connected;
//     }

//     PlcType plcType() const override {
//         return PlcType::Mitsubishi_EthernetTCP;
//     }

//     QString lastError() const override {
//         return m_last_msg;
//     }

//     QJsonObject toJson() const override ;
//     void setConfigJson(const QJsonObject& obj) override;

//     bool pushRequest(PlcRequest *request) override;

//     std::shared_ptr<MCCfg> McEthernetConfig();
//     void setMcEthernetConfig(MCCfg* cfg);

// private slots:
//     void onPollingTimeOut();
//     void socketReceived();

//     void check_device_changed();
//     void update_last_m_map();
//     void update_last_d_map();

// private:
//     void timerStart();
//     void timerStop();

// private:
//     QMutex m_mutex;
//     QTimer *m_polling_timer;
//     bool m_is_connected{false};
//     QString m_last_msg{""};

//     std::shared_ptr<MCCfg> m_config;
//     std::unique_ptr<MCFrameAbstract> m_frame;

//     std::shared_ptr<PlcRequest> m_current_request;

//     std::unique_ptr<PlcEthernetTcpPort> m_msg_interface;
//     std::shared_ptr<McContext> m_context;

//     QList<std::shared_ptr<PlcRequest>> m_request_queue;
//     QList<std::shared_ptr<PlcRequest>> m_polling_request_queue;

//     McDeviceMap m_device_map;
//     std::map<int, quint8> m_last_device_M_map;
//     std::map<int, qint16> m_last_device_D_map;
// };

// } // namespace vc::device

#endif // MITSUBISHI_ETHERNET_TCP_PLC_H
