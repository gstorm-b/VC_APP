#include "device_factory.h"

#include <QJsonDocument>

namespace vc::device {

namespace {

IDevice *createFromRegistryEntry(const QJsonObject &obj,
                                 DeviceType deviceType,
                                 QObject *parent)
{
    const DeviceRegistryEntry *entry = DeviceRegistry::find(obj, deviceType);
    if (!entry) {
        LOG_DEV_INFO << "DeviceFactory: unsupported device sub-type for family"
                     << DeviceTypeToString(deviceType);
        return nullptr;
    }

    if (!entry->available || !entry->creator) {
        LOG_DEV_INFO << "DeviceFactory: sub-type is unavailable"
                     << entry->subTypeValue;
        return nullptr;
    }

    return entry->creator(obj, parent);
}

} // namespace

IDevice *DeviceFactory::fromJson(const QJsonObject &obj, QObject *parent)
{
    if (!obj.contains(DEVICE_JSK_ID) ||
        !obj.contains(DEVICE_JSK_NAME) ||
        !obj.contains(DEVICE_JSK_TYPE) ||
        !obj.contains(DEVICE_JSK_CONFIG)) {
        LOG_DEV_INFO << "Cannot convert device from json, wrong json format.";
        LOG_DEV_INFO << QJsonDocument(obj).toJson();
        return nullptr;
    }

    return create(DeviceTypeFromString(obj[DEVICE_JSK_TYPE].toString()), obj, parent);
}

IDevice *DeviceFactory::create(const DeviceType &type,
                               const QJsonObject &obj,
                               QObject *parent)
{
    switch (type) {
    case DeviceType::Camera:
        return createCamera(obj, parent);
    case DeviceType::PLC:
        return createPlcDevice(obj, parent);
    case DeviceType::VisionOutput:
        return createVisionOutputDevice(obj, parent);
    case DeviceType::Robot:
        return createRobotDevice(obj, parent);
    case DeviceType::UserType:
    default:
        return nullptr;
    }
}

IDevice *DeviceFactory::createCamera(const QJsonObject &obj, QObject *parent)
{
    return createFromRegistryEntry(obj, DeviceType::Camera, parent);
}

IDevice *DeviceFactory::createPlcDevice(const QJsonObject &obj, QObject *parent)
{
    return createFromRegistryEntry(obj, DeviceType::PLC, parent);
}

IDevice *DeviceFactory::createVisionOutputDevice(const QJsonObject &obj,
                                                 QObject *parent)
{
    return createFromRegistryEntry(obj, DeviceType::VisionOutput, parent);
}

IDevice *DeviceFactory::createRobotDevice(const QJsonObject &obj, QObject *parent)
{
    return createFromRegistryEntry(obj, DeviceType::Robot, parent);
}

} // namespace vc::device
