#ifndef DEVICE_REGISTRY_H
#define DEVICE_REGISTRY_H

#include <QList>
#include <QJsonObject>
#include <QString>
#include <QStringList>

#include <functional>

#include "device/idevice.h"

namespace vc::device {

struct DeviceRegistryEntry {
    DeviceType deviceType{DeviceType::UserType};
    QString subTypeValue;
    QString displayName;
    QString configJsonKey;
    std::function<IDevice *(const QJsonObject &, QObject *)> creator;
    bool available{true};
};

class DeviceRegistry {
public:
    DeviceRegistry() = delete;
    ~DeviceRegistry() = delete;

    static const QList<DeviceRegistryEntry> &entries();
    static const DeviceRegistryEntry *find(DeviceType deviceType,
                                           const QString &subTypeValue);
    static const DeviceRegistryEntry *find(const QJsonObject &obj,
                                           DeviceType deviceType);
    static QStringList displayNamesFor(DeviceType deviceType,
                                       bool includeUnavailable = false);
};

} // namespace vc::device

#endif // DEVICE_REGISTRY_H
