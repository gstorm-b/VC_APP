#ifndef DEVICE_FACTORY_H
#define DEVICE_FACTORY_H

#include "device/idevice.h"
#include "device/device_registry.h"
#include <QJsonObject>
#include <QString>
#include <QSet>
#include <QMutex>

namespace vc::device {

class DeviceFactory {
public:
    DeviceFactory() = delete;
    ~DeviceFactory() = delete;
    DeviceFactory(const DeviceFactory&) = delete;
    DeviceFactory& operator=(const DeviceFactory&) = delete;

    static IDevice* fromJson(const QJsonObject& o,
                             QObject* parent = nullptr);

    static IDevice* create(const DeviceType& type,
                           const QJsonObject& obj,
                           QObject* parent = nullptr);

    static IDevice* createCamera(const QJsonObject& obj,
                                 QObject* parent = nullptr);
    static IDevice* createPlcDevice(const QJsonObject& obj,
                                    QObject* parent = nullptr);
    static IDevice* createVisionOutputDevice(const QJsonObject& obj,
                                             QObject* parent = nullptr);
    static IDevice* createRobotDevice(const QJsonObject& obj,
                                      QObject* parent = nullptr);
};

} // namespace vc::device

#endif // DEVICE_FACTORY_H
