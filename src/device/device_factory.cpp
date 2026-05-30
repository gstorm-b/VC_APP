#include "device_factory.h"
#include "device/idevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QDebug>
#include <QThread>

#include "device/camera/camera_device.h"
#include "device/plc/plc_device.h"
#include "device/plc/mc_protocol_device.h"
#include "device/camera/camera_basler_gige.h"
#include "device/output_device/vision_output_device.h"
#include "device/output_device/vision_tcpip_device.h"
#include "device/robot/robot_device.h"
#include "device/robot/kawasaki_robot_device.h"
#include "device/robot/nachi_robot_device.h"

namespace vc::device {

IDevice* DeviceFactory::fromJson(const QJsonObject& obj,
                                 QObject* parent) {

    if (!obj.contains(DEVICE_JSK_ID) ||
        !obj.contains(DEVICE_JSK_NAME) ||
        !obj.contains(DEVICE_JSK_TYPE) ||
        !obj.contains(DEVICE_JSK_CONFIG)) {

        LOG_DEV_INFO << "Cannot convert device from json, wrong json format.";
        LOG_DEV_INFO << QJsonDocument(obj).toJson();
        return nullptr;
    }

    const QString type  = obj[DEVICE_JSK_TYPE].toString();
    // IDevice* dev = create(DeviceTypeFromString(type), obj, parent);
    return create(DeviceTypeFromString(type), obj, parent);
}

IDevice* DeviceFactory::create(const DeviceType& type,
                               const QJsonObject& obj,
                               QObject* parent) {

    // qDebug() << "Device created from thread:" << QThread::currentThread();
    switch (type) {
        case vc::device::DeviceType::UserType:
            return nullptr;
        case vc::device::DeviceType::Camera:
            return createCamera(obj, parent);
        case vc::device::DeviceType::PLC:
            return createPlcDevice(obj, parent);
        case vc::device::DeviceType::VisionOutput:
            return createVisionOutputDevice(obj, parent);
        case vc::device::DeviceType::Robot:
            return createRobotDevice(obj, parent);
        default:
            return nullptr;
    }
    return nullptr;
}


IDevice* DeviceFactory::createCamera(const QJsonObject& obj, QObject* parent) {
    if (!obj.contains(DEVICE_JSK_CAM_TYPE)) {
        LOG_DEV_INFO << "Cannot convert camera device from json, unknown camera type.";
        return nullptr;
    }

    QString cam_type_str = obj[DEVICE_JSK_CAM_TYPE].toString();
    vc::device::CameraType cam_type = CameraTypeFromString(cam_type_str);

    switch (cam_type) {
    case vc::device::CameraType::Realsense:
        return nullptr;
    case vc::device::CameraType::BaslerGigE:
        return createBaslerGigeCamera(obj, parent);
    case vc::device::CameraType::BaslerUSB:
        return nullptr;
    default:
        break;
    }
    return nullptr;
}

IDevice* DeviceFactory::createBaslerGigeCamera(const QJsonObject& obj,
                                   QObject* parent) {

    QString deviceId = obj[DEVICE_JSK_ID].toString();
    QString deviceName = obj[DEVICE_JSK_NAME].toString();
    if (deviceId.isEmpty()) {
        return nullptr;
    }

    BaslerGigECamera* device = new BaslerGigECamera(deviceId, deviceName, parent);
    if (obj.contains(DEVICE_JSK_CONFIG) && obj[DEVICE_JSK_CONFIG].isObject()) {
        device->fromJson(obj);
    }

    return device;
}

IDevice* DeviceFactory::createPlcDevice(const QJsonObject& obj, QObject* parent) {
    const QJsonObject cfg = obj[DEVICE_JSK_CONFIG].toObject();
    if (!cfg.contains(DEVICE_JSK_PLC_TYPE)) {
        LOG_DEV_INFO << "Cannot convert PLC device from json, unknown sub-type.";
        return nullptr;
    }
    PlcType pt = PlcTypeFromString(cfg[DEVICE_JSK_PLC_TYPE].toString());
    switch (pt) {
    case PlcType::MitsubishiMc: return createMcProtocolDevice(obj, parent);
    case PlcType::PlcTypeNone:
        return nullptr;
    }
    return nullptr;
}

IDevice* DeviceFactory::createMcProtocolDevice(const QJsonObject& obj, QObject* parent) {
    QString deviceId = obj[DEVICE_JSK_ID].toString();
    QString deviceName = obj[DEVICE_JSK_NAME].toString();
    if (deviceId.isEmpty()) {
        return nullptr;
    }

    /// TODO: change fromjson method
    McProtocolDevice *device = new McProtocolDevice(deviceId, deviceName, parent);
    if (obj.contains(DEVICE_JSK_CONFIG) && obj[DEVICE_JSK_CONFIG].isObject()) {
        device->fromJson(obj);
    }
    return device;
}

IDevice* DeviceFactory::createVisionOutputDevice(const QJsonObject& obj, QObject* parent) {
    const QJsonObject cfg = obj[DEVICE_JSK_CONFIG].toObject();
    if (!cfg.contains(DEVICE_JSK_VOUT_TYPE)) {
        LOG_DEV_INFO << "Cannot convert vision-output device from json, unknown sub-type.";
        return nullptr;
    }
    VisionOutputType vt = VisionOutputTypeFromString(cfg[DEVICE_JSK_VOUT_TYPE].toString());
    switch (vt) {
    case VisionOutputType::VisionTCPIP:  return createVisionTcpipDevice(obj, parent);
    case VisionOutputType::VisionSerial: return nullptr;
    case VisionOutputType::VisionOutputTypeNone:
        return nullptr;
    }
    return nullptr;
}

IDevice* DeviceFactory::createVisionTcpipDevice(const QJsonObject& obj, QObject* parent) {
    QString deviceId = obj[DEVICE_JSK_ID].toString();
    QString deviceName = obj[DEVICE_JSK_NAME].toString();
    if (deviceId.isEmpty()) {
        return nullptr;
    }

    auto *device = new VisionTcpipDevice(deviceId, deviceName, parent);
    if (obj.contains(DEVICE_JSK_CONFIG) && obj[DEVICE_JSK_CONFIG].isObject()) {
        device->fromJson(obj);
    }
    return device;
}

IDevice* DeviceFactory::createRobotDevice(const QJsonObject& obj, QObject* parent) {
    const QJsonObject cfg = obj[DEVICE_JSK_CONFIG].toObject();
    if (!cfg.contains(DEVICE_JSK_ROBOT_TYPE)) {
        LOG_DEV_INFO << "Cannot convert robot device from json, unknown robot type.";
        return nullptr;
    }
    RobotType rt = RobotTypeFromString(cfg[DEVICE_JSK_ROBOT_TYPE].toString());
    switch (rt) {
    case RobotType::Kawasaki: return createKawasakiRobot(obj, parent);
    case RobotType::Nachi:    return createNachiRobot(obj, parent);
    case RobotType::Huayan:   return nullptr;
    case RobotType::RobotTypeNone:
        return nullptr;
    }
    return nullptr;
}

IDevice* DeviceFactory::createKawasakiRobot(const QJsonObject& obj, QObject* parent) {
    QString deviceId = obj[DEVICE_JSK_ID].toString();
    QString deviceName = obj[DEVICE_JSK_NAME].toString();
    if (deviceId.isEmpty()) {
        return nullptr;
    }

    auto *device = new KawasakiRobotDevice(deviceId, deviceName, parent);
    if (obj.contains(DEVICE_JSK_CONFIG) && obj[DEVICE_JSK_CONFIG].isObject()) {
        device->fromJson(obj);
    }
    return device;
}

IDevice* DeviceFactory::createNachiRobot(const QJsonObject& obj, QObject* parent) {
    QString deviceId = obj[DEVICE_JSK_ID].toString();
    QString deviceName = obj[DEVICE_JSK_NAME].toString();
    if (deviceId.isEmpty()) {
        return nullptr;
    }

    auto *device = new NachiRobotDevice(deviceId, deviceName, parent);
    if (obj.contains(DEVICE_JSK_CONFIG) && obj[DEVICE_JSK_CONFIG].isObject()) {
        device->fromJson(obj);
    }
    return device;
}

} // namespace vc::IDevice
