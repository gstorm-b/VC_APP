#ifndef VISION_OUTPUT_CONFIG_H
#define VISION_OUTPUT_CONFIG_H

#include "device/idevice_config.h"
#include "logger/app_logger.h"

#include <QJsonObject>
#include <QString>

namespace vc::device {

// Family-level sub-type dispatch handle. Mirrors CameraType / RobotType.
// Concrete vendors register a value here; DeviceFactory::createVisionOutput()
// switches on this enum to pick the concrete subclass.
enum VisionOutputType : int {
    VisionOutputTypeNone,
    VisionTCPIP,
    VisionSerial,   // placeholder for future transport
};

inline QString VisionOutputTypeToString(VisionOutputType t) {
    switch (t) {
    case VisionTCPIP:            return QStringLiteral("VisionTCPIP");
    case VisionSerial:           return QStringLiteral("VisionSerial");
    case VisionOutputTypeNone:   return QString();
    }
    return QString();
}

inline VisionOutputType VisionOutputTypeFromString(const QString &t) {
    if (t == QLatin1String("VisionTCPIP"))  return VisionTCPIP;
    if (t == QLatin1String("VisionSerial")) return VisionSerial;
    return VisionOutputTypeNone;
}

// =====================================================================
// VisionOutputDeviceCfg — abstract config for the vision-output family
// =====================================================================
//
// Carries only the family-level dispatch field. Concrete configs
// (VisionTcpipDeviceCfg, future VisionSerialDeviceCfg, …) inherit and add
// their transport-specific Q_PROPERTYs.
//
class VisionOutputDeviceCfg : public IDeviceCfg {
public:
    virtual VisionOutputType visionOutputType() const = 0;

    DeviceType deviceType() const override {
        return DeviceType::VisionOutput;
    }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj[DEVICE_JSK_VOUT_TYPE] =
            VisionOutputTypeToString(this->visionOutputType());
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        if (!obj.contains(DEVICE_JSK_VOUT_TYPE)) {
            LOG_DEV_ERR << "VisionOutputDeviceCfg: missing VisionOutputType key";
            return false;
        }
        if (visionOutputType() !=
            VisionOutputTypeFromString(obj[DEVICE_JSK_VOUT_TYPE].toString())) {
            LOG_DEV_ERR << "VisionOutputDeviceCfg: type mismatch -"
                        << obj[DEVICE_JSK_VOUT_TYPE].toString();
            return false;
        }
        return true;
    }
};

} // namespace vc::device

#endif // VISION_OUTPUT_CONFIG_H
