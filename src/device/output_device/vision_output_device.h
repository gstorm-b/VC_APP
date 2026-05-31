#ifndef VISION_OUTPUT_DEVICE_H
#define VISION_OUTPUT_DEVICE_H

#include "device/idevice.h"
#include "device/device_capabilities.h"
#include "device/output_device/vision_output_config.h"
#include "device/output_device/vision_output_request.h"

#define VISION_OUTPUT_TYPE_TCPIP    "VisionTCPIP"
#define VISION_OUTPUT_TYPE_SERIAL   "VisionSerial"

namespace vc::device {

// Family-level sub-type dispatch handle. Mirrors CameraType / RobotType.
// Concrete vendors register a value here; DeviceFactory::createVisionOutput()
// switches on this enum to pick the concrete subclass.
// enum VisionOutputType {
//     VisionOutputTypeNone,
//     VisionTCPIP,
//     VisionSerial,   // placeholder for future transport
// };

// =====================================================================
// VisionOutputDevice — abstract base for the vision-output family
// =====================================================================
//
// Concrete vendors (VisionTcpipDevice, future VisionSerialDevice, …) inherit
// from this base. The base only carries the family-level dispatch
// (visionOutputType()) and the family JSON header; transport-specific
// surface (TCP servers / serial port / heartbeat) lives entirely on the
// concrete subclass.
//
class VisionOutputDevice : public IDevice, public IResultOutputDevice {
    Q_OBJECT

public:
    explicit VisionOutputDevice(QString id, QString name, QObject* parent = nullptr)
        : IDevice(id, name, parent) {}

    DeviceType deviceType() const override {
        return DeviceType::VisionOutput;
    }

    virtual VisionOutputType visionOutputType() const = 0;

    QJsonObject toJson() const override {
        QJsonObject obj = IDevice::toJson();
        obj.insert(DEVICE_JSK_VOUT_TYPE,
                   VisionOutputTypeToString(this->visionOutputType()));
        return obj;
    }
};

} // namespace vc::device

#endif // VISION_OUTPUT_DEVICE_H
