#include "device_factory.h"
#include "device/idevice.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QDebug>

#include "device/camera/camera_device.h"
#include "device/plc/mc_protocol_device.h"
#include "device/camera/camera_basler_gige.h"

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


    switch (type) {
        case vc::device::DeviceType::UserType:
            return nullptr;
        case vc::device::DeviceType::Camera:
            return createCamera(obj, parent);
        case vc::device::DeviceType::McDevice:
            return createMcProtocolDevice(obj, parent);
        case vc::device::DeviceType::TCPIP_DEVICE:
            return nullptr;
        case vc::device::DeviceType::Robot:
            return nullptr;
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
        // McProtocolConfig *config = new McProtocolConfig();
        // config->fromJson(obj[DEVICE_JSK_CONFIG].toObject());
        // device->setDeviceConfig(config);
    }
    return device;
}

} // namespace vc::IDevice
