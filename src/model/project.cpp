#include "project.h"
#include "device/device_factory.h"
#include "model/task_factory.h"

#include <QTimer>
#include <QEventLoop>

namespace vc::model {

Project::Project(QObject* parent)
    : QObject(parent) {

    m_dvManager = std::make_shared<device::DeviceManager>(this);

    connect(m_dvManager.get(), &device::DeviceManager::devicesChanged,
            this, &Project::projectModificationOccurred);
    connect(m_dvManager.get(), &device::DeviceManager::deviceModified,
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

    if (m_occupieTaskdNames.contains(task->name())){
        return false;
    }

    std::shared_ptr<vc::model::ITask> task_ptr;
    task_ptr.reset(task);
    task_ptr->setProject(this);
    m_tasks.insert(task_ptr->id(), task_ptr);
    m_occupieTaskdNames.insert(task_ptr->name());

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

std::shared_ptr<vc::model::ITask> Project::taskById(const QString& id) const {
    if (m_tasks.contains(id)) {
        return m_tasks.value(id, nullptr);
    }
    return nullptr;
}

bool Project::changeTaskName(const QString& id, const QString &name) {
    if (m_occupieTaskdNames.contains(name)) {
        return false;
    }

    if (!m_tasks.contains(id)) {
        return false;
    }

    std::shared_ptr<ITask> task = m_tasks.value(id);
    QString old_name = task->id();
    task->setName(name);
    m_occupieTaskdNames.remove(old_name);
    emit taskModified(id);
    return false;
}

bool Project::isTaskNameOccupied(const QString& name) const {
    return m_occupieTaskdNames.contains(name);
}

std::shared_ptr<device::DeviceManager> Project::deviceManager() {
    return m_dvManager;
}

std::shared_ptr<vc::device::IDevice> Project::deviceById(const QString& id) {
    return m_dvManager->getCurrentDevices().value(id, nullptr);

    // QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_start = m_dvManager->getCurrentDevices().cbegin();
    // QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_end = m_dvManager->getCurrentDevices().cend();
    // while (it_start != it_end) {
    //     if (it_start.key() == id) {
    //         return it_start.value();
    //     }
    //     it_start++;
    // }
    // return std::shared_ptr<vc::device::IDevice>(nullptr);
}

QJsonObject Project::toJson() const {
    // get device json data
    QJsonArray deviceArr;
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_device = m_dvManager->getCurrentDevices().cbegin();
    QMap<QString, std::shared_ptr<vc::device::IDevice>>::const_iterator it_device_end = m_dvManager->getCurrentDevices().cend();
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
        m_dvManager->reserveDevice(id, name, dv);
    }

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
    }
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


}
