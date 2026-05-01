#ifndef MC_PROTOCOL_CONFIG_H
#define MC_PROTOCOL_CONFIG_H

#include "device/idevice_config.h"
#include "device/plc/mc_context.h"
#include "device/plc/mc_msg_interface.h"
#include "device/plc/mc_context_factory.h"

namespace vc::device {

class McProtocolConfig : public IDeviceCfg {
    Q_GADGET

public:
    explicit McProtocolConfig()
        : IDeviceCfg() {}

    const QMetaObject &getMetaObject() const override {        
        return vc::device::McProtocolConfig::staticMetaObject;
    }

    DeviceType deviceType() const override {
        return DeviceType::PLC;
    }

    McFrameType currentFrameType() const {
        if (!m_context) {
            return McFrameType::Frame_User;
        }
        return m_context->frameType();
    }

    bool setContext(McContext *ctx) {
        if (!ctx) {
            return false;
        }
        m_context.reset(ctx->clone());
        return true;
    }

    McContext* context() {
        return m_context.get();
    }

    bool configMcProtocol(McFrameType frame_type, McDataCode code = McDataCode::Binary) {
        std::shared_ptr<McContext> temp = Factory::contextFactory(frame_type, code);
        if (!temp) {
            return false;
        }
        m_context = temp;
        return true;
    }

    QJsonObject toJson() const override {
        QJsonObject obj;
        if (m_context) {
            obj[DEVICE_JSK_MC_FRAME] = McFrameTypeToString(m_context->frameType());
            obj[DEVICE_JSK_MC_CONTEXT] = m_context->toJson();
        } else {
            obj[DEVICE_JSK_MC_FRAME] = "";
        }
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        McFrameType frame_type = McFrameTypeFromString(obj[DEVICE_JSK_MC_FRAME].toString());
        m_context = Factory::contextFactory(frame_type);
        if (m_context) {
            m_context->fromJson(obj[DEVICE_JSK_MC_CONTEXT].toObject());
            return true;
        }
        return false;
    }

    IDeviceCfg* clone() override {
        McProtocolConfig* new_ptr = new McProtocolConfig(*this);
        if (this->m_context) {
            McContext* ctx = this->m_context.get();
            new_ptr->setContext(ctx);
        }
        return new_ptr;
    }

private:
    std::shared_ptr<McContext> m_context;
};

}

#endif // MC_PROTOCOL_CONFIG_H
