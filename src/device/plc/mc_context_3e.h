#ifndef MC_CONTEXT_3E_H
#define MC_CONTEXT_3E_H

#include "mc_context.h"
#include <QObject>
#include <QMetaType>

namespace vc::device {

class Context_Mc3E : public McContext {
    Q_GADGET

public:
    Context_Mc3E() {
        m_data_code = McDataCode::Binary;
        m_msg_cfg = std::make_shared<McMsgEthernetTcpCfg>();
    }

    Context_Mc3E(McDataCode data_code) {
        m_data_code = data_code;
        m_msg_cfg = std::make_shared<McMsgEthernetTcpCfg>();
    }

    ~Context_Mc3E() {

    }

    const QMetaObject &getMetaObject() const override {
        return vc::device::Context_Mc3E::staticMetaObject;
    }

    McFrameType frameType() const override {
        return McFrameType::Frame_3E;
    }

    McMsgItfType msgIntefaceType() const override {
        return McMsgItfType::EthernetTCPIP;
    }

    McContext* clone() const override {
        return new Context_Mc3E(*this);
    }

    QJsonObject toJson() const override {
        QJsonObject obj = McContext::toJson();
        obj["NetworkNo"] = m_network;
        obj["PcNo"] = m_pc;
        obj["RequestDestNo"] = m_destModuleIo;
        obj["RequestStationNo"] = m_multiStation;
        obj["MonitoringTime"] = m_monitoringTime;

        if (m_msg_cfg) {
            obj["MsgConfig"] = m_msg_cfg->toJson();
        }
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        m_network           = obj["NetworkNo"].toInt(0x00);
        m_pc                = obj["PcNo"].toInt(0xFF);
        m_destModuleIo    = obj["RequestDestNo"].toInt(0x03FF);
        m_multiStation     = obj["RequestStationNo"].toInt(0x00);
        m_monitoringTime   = obj["MonitoringTime"].toInt(0x04);

        if (!m_msg_cfg) {
            m_msg_cfg = std::make_shared<McMsgEthernetTcpCfg>();
        }
        m_msg_cfg->fromJson(obj["MsgConfig"].toObject());
        return McContext::fromJson(obj);
    }

public:
    // Frame
    // SUB_HEADER
    // fixed for 3E frame: 0x5000
    // REQUEST DESTINATION NETWORK .NO
    // if connect directly with station, normal value is 0x00
    // REQUEST DESTINATION STATION .NO
    // if conenct directly with station, normal value is 0xFF
    // REQUEST DESTINATION MODULE IO .NO
    // refer to document for more information, if connect directly this value is 0x03FF
    // REQUEST DESTINATION MULTIDROP STATION .NO
    // normal value is 0x00
    // REQUEST DATA LENGTH
    // total number of bytes [MONITORING TIME + REQUEST DATA]
    // MONITORING TIME
    //
    // REQUEST DATA
    // SUB_HEADER + ACCESS_ROUTE + REQUEST_DATA_LEN + MONITORING_TIME + REQUEST_DATA

    // fixed sub header of MC 3E is 0x5000
    const quint16 m_subHeader = 0x5000;
    // network No. of an access target, 0 <= network <= 255
    quint8 m_network = 0x00;
    // pc is network module station No. of target access 0 <= pc <= 255
    quint8 m_pc = 0xFF;
    // destination module io number
    quint16 m_destModuleIo = 0x03FF;
    // multidrop station number
    quint8 m_multiStation = 0x00;
    // monitoring time
    quint16 m_monitoringTime = 0x04;
};

}

Q_DECLARE_METATYPE(vc::device::Context_Mc3E)

#endif // MC_CONTEXT_3E_H
