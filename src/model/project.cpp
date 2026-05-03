#include "project.h"
#include "device/device_factory.h"
#include "model/task_factory.h"

#include <QTimer>
#include <QEventLoop>

namespace vc::model {

Project::Project(QObject* parent)
    : QObject(parent) {

    m_deviceManager = std::make_shared<device::DeviceManager>(this);

    connect(m_deviceManager.get(), &device::DeviceManager::devicesChanged,
            this, &Project::projectModificationOccurred);
    connect(m_deviceManager.get(), &device::DeviceManager::deviceModified,
            this, [this]() {
        emit this->projectModificationOccurred();
    });

    connect(this, &vc::model::Project::taskModified,
            this, [this]() {
        emit this->projectModificationOccurred();
    });
}

Project::~Project() {

}

void Project::setName(const QString& v) {
    if (m_name == v) return;
    m_name = v; emit nameChanged();
}

void Project::setDescription(const QString &v) {
    m_description = v;
}

void Project::setAuthor(const QString& v) {
    if (m_author == v) return;
    m_author = v; emit authorChanged();
}

void Project::setVersion(const QString& v) {
    if (m_version == v) return;
    m_version = v; emit versionChanged();
}

void Project::setCreatedAt(const QString& v) {
    m_createdAt = v; emit createdAtChanged();
}

void Project::setUpdatedAt(const QString& v) {
    m_updatedAt = v; emit updatedAtChanged();
}

bool Project::addTask(ITask* task) {
    if (!task) {
        return false;
    }

    if (m_occupiedTaskNames.contains(task->name())){
        return false;
    }

    std::shared_ptr<vc::model::ITask> task_ptr;
    task_ptr.reset(task);
    task_ptr->setProject(this);
    m_tasks.insert(task_ptr->id(), task_ptr);
    m_occupiedTaskNames.insert(task_ptr->name());

    // connect signal
    connect(task_ptr.get(), &vc::model::ITask::configChanged, this, [this, task_ptr]() {
        emit this->taskModified(task_ptr->id());
    });

    connect(task_ptr.get(), &vc::model::ITask::configChanged,
            this, &Project::projectModificationOccurred);

    emit tasksChanged();
    emit taskCreated(task_ptr->id());
    return true;
}

bool Project::removeTask(const QString& id) {
    if (m_tasks.contains(id)) {
        m_tasks.remove(id);
        emit tasksChanged();
        emit taskDeleted(id);
        return true;
    }
    return false;
}

bool Project::assignDeviceToTask(const QString &deviceId, const QString &taskId) {
    std::shared_ptr<vc::model::ITask> task = this->taskById(taskId);
    if (!task) return false;

    std::shared_ptr<vc::device::IDevice> device = this->deviceById(deviceId);
    if (!device) return false;

    task->assignDevice(deviceId);
    device->setAssignedTaskId(taskId);
    return true;
}

bool Project::unassignDeviceFromTask(const QString &deviceId, const QString &taskId) {
    std::shared_ptr<vc::model::ITask> task = this->taskById(taskId);
    if (!task) return false;

    std::shared_ptr<vc::device::IDevice> device = this->deviceById(deviceId);
    if (!device) return false;

    task->unassignDevice(deviceId);
    device->setAssignedTaskId("");
    return true;
}

std::shared_ptr<vc::model::ITask> Project::taskById(const QString& id) const {
    if (m_tasks.contains(id)) {
        return m_tasks.value(id, nullptr);
    }
    return nullptr;
}

bool Project::changeTaskName(const QString& id, const QString &name) {
    if (m_occupiedTaskNames.contains(name)) {
        return false;
    }

    if (!m_tasks.contains(id)) {
        return false;
    }

    std::shared_ptr<ITask> task = m_tasks.value(id);
    QString old_name = task->id();
    task->setName(name);
    m_occupiedTaskNames.remove(old_name);
    emit taskModified(id);
    return false;
}

bool Project::isTaskNameOccupied(const QString& name) const {
    return m_occupiedTaskNames.contains(name);
}

std::shared_ptr<device::DeviceManager> Project::deviceManager() {
    return m_deviceManager;
}

std::shared_ptr<vc::device::IDevice> Project::deviceById(const QString& id) {
    return m_deviceManager->getCurrentDevices().value(id, nullptr);
}

QJsonObject Project::toJson() const {
    // get device json data
    QJsonArray deviceArr;
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_device = m_deviceManager->getCurrentDevices().cbegin();
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_device_end = m_deviceManager->getCurrentDevices().cend();
    while (it_device != it_device_end) {
        deviceArr.append(it_device.value()->toJson());
        it_device++;
    }

    // get task json data
    QJsonArray taskArr;
    QMap<QString, std::shared_ptr<vc::model::ITask>>::const_iterator it_task = m_tasks.cbegin();
    while (it_task != m_tasks.cend()) {
        taskArr.append(it_task.value()->toJson());
        it_task++;
    }

    qDebug() << deviceArr;

    return QJsonObject {
                       { "name",    m_name    },
                       { "version", m_version },
                       { "devices", deviceArr },
                       { "tasks",   taskArr   },
                       };
}

bool Project::fromJson(const QJsonObject &json) {
    if (json.isEmpty()) {
        return false;
    }

    m_name = json["name"].toString();
    m_version = json["version"].toString();
    QJsonArray taskArr = json["tasks"].toArray();
    QJsonArray deviceArr = json["devices"].toArray();

    // convert devices from json
    for (int idx=0;idx<deviceArr.size();idx++) {
        if (!deviceArr[idx].isObject()) {
            continue;
        }

        QJsonObject obj = deviceArr[idx].toObject();
        std::shared_ptr<device::IDevice> dv(device::DeviceFactory::fromJson(obj));
        if (!dv) {
            continue;
        }

        QString id = obj[DEVICE_JSK_ID].toString();
        QString name = obj[DEVICE_JSK_NAME].toString();
        m_deviceManager->reserveDevice(id, name, dv);
        LOG_USER_INFO << "Device loaded from JSON. ID: " << id << ", Name: " << name;
    }

    // convert tasks from json
    for (int idx=0;idx<taskArr.size();idx++) {
        if (!taskArr[idx].isObject()) {
            continue;
        }

        QJsonObject obj = taskArr[idx].toObject();
        model::ITask* task = model::TaskFactory::fromJson(obj);
        if (!task) {
            continue;
        }

        this->addTask(task);
        LOG_USER_INFO << "Task loaded from JSON. ID: " << task->id() << ", Name: " << task->name();
    }

    // assign devices to tasks
    const QMap<QString, std::shared_ptr<vc::device::IDevice>>& devices = m_deviceManager->getCurrentDevices();
    QStringList releaseDeviceIds;
    for (const std::shared_ptr<vc::device::IDevice>& device : devices) {
        qDebug() << device->id() << device->assignedTaskId();
        QString assignedTaskId = device->assignedTaskId();
        if (assignedTaskId.isEmpty()) {
            releaseDeviceIds.append(device->id());
            LOG_USER_ERR << "Device " << device->id() << " has no assigned task, deleting device.";
            continue;
        }

        std::shared_ptr<vc::model::ITask> task = this->taskById(assignedTaskId);
        if (task) {
            task->assignDevice(device->id());
            LOG_USER_INFO << "Assigned device " << device->id() << " to task " << assignedTaskId;
        }
    }

    for (const QString &deviceId : releaseDeviceIds) {
        m_deviceManager->releaseDevice(deviceId);
    }

    LOG_USER_INFO << "Project loaded from JSON successfully. Name: " << m_name << ", Version: " << m_version
                 << ", Devices: " << devices.size() << ", Tasks: " << m_tasks.size();
    // convert succeesfully
    return true;
}

// QMap<QString, QMap<QString, cv::Mat>> Project::toImageMaps() const {
//     QMap<QString, QMap<QString, cv::Mat>> maps;
//     auto task_it = m_tasks.cbegin();
//     while (task_it != m_tasks.cend()) {
//         QMap<QString, cv::Mat> mapping = task_it.value()->getTaskImageMap();
//         maps.insert(task_it.key(), mapping);
//         task_it++;
//     }
//     return maps;
// }

// bool Project::fromImageMaps(const QMap<QString, QMap<QString, cv::Mat>> &mapping) {
//     auto map_it = mapping.cbegin();
//     while (map_it != mapping.cend()) {
//         std::shared_ptr<vc::model::ITask> task = m_tasks.value(map_it.key());
//         if (task) {
//             QMap<QString, cv::Mat> image_mat = map_it.value();
//             task->loadTaskImageMap(image_mat);
//         }
//         map_it++;
//     }
//     return true;
// }

bool Project::moveDeviceToTask(const QString &deviceId,
                               const QString &fromTaskId,
                               const QString &toTaskId)
{
    auto fromTask = taskById(fromTaskId);
    auto toTask   = taskById(toTaskId);
    auto device   = deviceById(deviceId);
    if (!fromTask || !toTask) return false;
    if (!fromTask->hasDevice(deviceId)) return false;
    if (toTask->hasDevice(deviceId))   return false;

    fromTask->unassignDevice(deviceId);
    toTask->assignDevice(deviceId);
    device->setAssignedTaskId(toTaskId);
    emit projectModificationOccurred();
    return true;
}

}
