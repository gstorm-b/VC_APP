#ifndef ITASK_H
#define ITASK_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QVector>
#include <QStringList>

#include <opencv2/opencv.hpp>

#include "task_define.h"
#include "itask_config.h"
#include "logger/app_logger.h"

namespace vc::model {

class Project;

class ITask : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_CLASSINFO("name_name", "Task name")

    // Q_PROPERTY(QString id READ id CONSTANT)
    // Q_CLASSINFO("task_name", "Task ID")
    // Q_PROPERTY(bool owned READ isOwned NOTIFY cameraSourceTypeChanged)

public:
    explicit ITask(QString init_id = "", QObject* parent = nullptr)
        : QObject(parent) {

        // create id, cannot change id after construction
        if (init_id.isEmpty()) {
            m_id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        } else {
            m_id = init_id;
        }
    }

    virtual ~ITask() = default;

    virtual TaskType taskType() const = 0;
    virtual bool isValid() const = 0;

    QString id() const {
        return m_id;
    }

    QString name() const {
        return m_name;
    }

    void setName(const QString& v) {
        if (m_name == v) {
            return;
        }
        m_name = v;
        emit nameChanged();
    }

    CameraSourceType cameraSourceType() const {
        return m_cameraSourceType;
    }

    void setCameraSourceType(CameraSourceType v) {
        m_cameraSourceType = v;
        emit cameraSourceTypeChanged();
    }

    bool isOwned()  const {
        return m_cameraSourceType == CameraSourceType::Source_Owned;
    }

    bool isShared() const {
        return m_cameraSourceType == CameraSourceType::Source_Shared;
    }

    QString ownedCameraId() const {
        return m_ownedCameraId;
    }

    void setOwnedCameraId(const QString& v) {
        m_ownedCameraId = v;
    }

    // ── Device ownership ───────────────────────────────
    void assignDevice(const QString &deviceId) {
        if (m_assignedDeviceIds.contains(deviceId)) return;
        m_assignedDeviceIds.append(deviceId);
        emit devicesChanged();
    }

    void unassignDevice(const QString &deviceId) {
        if (m_assignedDeviceIds.removeOne(deviceId))
            emit devicesChanged();
    }

    bool hasDevice(const QString &deviceId) const {
        return m_assignedDeviceIds.contains(deviceId);
    }

    QStringList assignedDeviceIds() const {
        return m_assignedDeviceIds;
    }

    virtual void setTaskConfig(ITaskConfig *cfg) {
        m_abstract_cfg = cfg;
        emit configChanged();
    }

    virtual ITaskConfig* taskConfig() {
        return m_abstract_cfg->copy();
    }

    virtual QJsonObject toJson() const {
        QJsonObject obj;
        obj["id"] = m_id;
        obj["name"] = m_name;
        obj["taskType"] = qenumToString(taskType());
        obj["cameraSourceType"] = qenumToString(m_cameraSourceType);
        obj["ownedCameraId"] = m_ownedCameraId;

        QJsonArray devIds;
        for (const QString &id : m_assignedDeviceIds)
            devIds.append(id);
        obj["assignedDeviceIds"] = devIds;

        if (m_abstract_cfg) {
            obj["taskConfig"] = m_abstract_cfg->toJson();
        }
        return obj;
    }


    virtual bool fromJson(const QJsonObject& obj) {
        if (!obj.contains("id") ||
            !obj.contains("name") ||
            !obj.contains("taskType") ||
            !obj.contains("taskConfig")) {
            LOG_DEV_ERR << "Cannot convert task from json, wrong json format.";
            LOG_DEV_ERR << QJsonDocument(obj).toJson();
            return false;
        }

        TaskType task_type = stringToQEnum(obj["taskType"].toString(), TaskType::UndefineTask);
        if (task_type != this->taskType()) {
            LOG_DEV_ERR << "Cannot convert task from json, wrong task type.";
            LOG_DEV_ERR << obj["taskType"].toString();
            return false;
        }

        m_id = obj["id"].toString();
        m_name = obj["name"].toString();

        if (obj.contains("assignedDeviceIds")) {
            m_assignedDeviceIds.clear();
            for (const QJsonValue &v : obj["assignedDeviceIds"].toArray())
                m_assignedDeviceIds.append(v.toString());
        }

        if (m_abstract_cfg) {
            return m_abstract_cfg->fromJson(obj["taskConfig"].toObject());
        }
        return true;
    }

    virtual QMap<QString, cv::Mat> getTaskImageMap() {
        return QMap<QString, cv::Mat>();
    }

    virtual bool loadTaskImageMap(QMap<QString, cv::Mat> &mapping) {
        return false;
    }

    virtual void setProject(Project *proj) {
        m_proj = proj;
    }

    virtual Project* project() {
        return m_proj;
    }


signals:
    void nameChanged();
    void configChanged();
    void cameraSourceTypeChanged();
    void patternsChanged();
    void devicesChanged();

private:
    QString m_id;
    QString m_name;
    CameraSourceType m_cameraSourceType = CameraSourceType::Source_Owned;
    QString m_ownedCameraId;
    QStringList m_assignedDeviceIds;
    ITaskConfig *m_abstract_cfg{nullptr};
    Project *m_proj{nullptr};
};

} // namespace vc::model
#endif // ITASK_H
