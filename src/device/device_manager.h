#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <QObject>
#include <QJsonObject>
#include <QString>
#include <QSet>
#include <QMutex>
#include <set>

#include "device/idevice.h"

namespace vc::model {
    class Project;
}

namespace vc::device {

/**
 * Device manager instance create by Project,
 * Because project manage DeviceManager by shared pointer
 * DeviceManager will always release by Project Destructor.
 */

class DeviceManager : public QObject {
    Q_OBJECT
public:
    explicit DeviceManager(vc::model::Project* proj = nullptr, QObject *parent = nullptr);
    ~DeviceManager();

    vc::model::Project* project();

    bool isLimitReached(DeviceType type) const;
    bool reserveDevice(const QString &id, const QString &name, std::shared_ptr<IDevice> device);
    void releaseDevice(const QString &id);
    void releasePendingId(const QString &id);
    QString allocatePendingId();
    bool commitDevice(const QString &id, const QString &name, std::shared_ptr<IDevice> device);
    QString getDeviceName(const QString &id);
    bool isNameExists(const QString &name);
    bool isOccupied(const QString &id);
    std::shared_ptr<IDevice> deviceById(const QString &id);

    bool changeDeviceName(const QString &id, const QString new_name);

    QMap<QString, QString> commDevices();
    QMap<QString, QString> outputDevices();
    QMap<QString, QString> cameraDevices();

    QStringList commDevicesNameList();
    QStringList outputDevicesNameList();
    QStringList cameraDevicesNameList();

    QStringList& getDeviceTypeList();
    QStringList& getSubDeviceTypeList(DeviceType type);

    const QMap<QString, std::shared_ptr<IDevice>>& getCurrentDevices() {
        return deviceInstances;
    }

signals:
    void devicesChanged();
    void deviceModified(QString id);
    void deviceCreated(QString id);
    void deviceDeleted(QString id);

private:
    QString generateId();
    void deviceNameCheck(IDevice* device);
    // Forwards a device's configChanged signal to deviceModified(id). Called by
    // both creation paths (reserveDevice on project load, commitDevice via the
    // Add-Device wizard) so config edits mark the project modified consistently.
    void connectConfigChanged(const std::shared_ptr<IDevice> &device);

private:
    QStringList dvTypeStrings;
    QMap<DeviceType, QStringList> subDeviceTypeLists;

    QString lastMsg;

    QMutex mutex;
    int currentMaxId;
    std::set<int> freedIds;
    QSet<QString> occupiedNames;

    QMap<QString, std::shared_ptr<IDevice>> deviceInstances;
    // QMap<QString, QThread> deviceWorkers;

    QMap<DeviceType, int> maxLimits;
    QMap<DeviceType, int> currentCounts;

    vc::model::Project* parent_proj{nullptr};
};

} // namespace vc::device

#endif // DEVICE_MANAGER_H
