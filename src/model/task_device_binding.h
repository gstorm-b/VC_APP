#ifndef TASK_DEVICE_BINDING_H
#define TASK_DEVICE_BINDING_H

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QMap>
#include <QString>
#include <utility>

namespace vc::model {

enum class TaskDeviceRole {
    Unknown,
    PrimaryPlc,
    VisionOutput,
    CameraNumber,
};

inline QString taskDeviceRoleToString(TaskDeviceRole role)
{
    switch (role) {
    case TaskDeviceRole::PrimaryPlc:
        return QStringLiteral("primary_plc");
    case TaskDeviceRole::VisionOutput:
        return QStringLiteral("vision_output");
    case TaskDeviceRole::CameraNumber:
        return QStringLiteral("camera_number");
    case TaskDeviceRole::Unknown:
        return QString();
    }
    return QString();
}

inline TaskDeviceRole taskDeviceRoleFromString(const QString &value)
{
    if (value == QStringLiteral("primary_plc")) return TaskDeviceRole::PrimaryPlc;
    if (value == QStringLiteral("vision_output")) return TaskDeviceRole::VisionOutput;
    if (value == QStringLiteral("camera_number")) return TaskDeviceRole::CameraNumber;
    return TaskDeviceRole::Unknown;
}

class TaskDeviceBinding {
public:
    TaskDeviceBinding() = default;

    TaskDeviceBinding(TaskDeviceRole role, QString deviceId, int cameraNumber = 0)
        : m_role(role), m_deviceId(std::move(deviceId)), m_cameraNumber(cameraNumber) {}

    TaskDeviceRole role() const { return m_role; }
    QString deviceId() const { return m_deviceId; }
    int cameraNumber() const { return m_cameraNumber; }

    QJsonObject toJson() const
    {
        QJsonObject obj;
        obj["role"] = taskDeviceRoleToString(m_role);
        obj["deviceId"] = m_deviceId;
        if (m_role == TaskDeviceRole::CameraNumber)
            obj["cameraNumber"] = m_cameraNumber;
        return obj;
    }

    bool fromJson(const QJsonObject &obj)
    {
        m_role = taskDeviceRoleFromString(obj["role"].toString());
        m_deviceId = obj["deviceId"].toString();
        m_cameraNumber = obj["cameraNumber"].toInt(0);
        return m_role != TaskDeviceRole::Unknown;
    }

private:
    TaskDeviceRole m_role{TaskDeviceRole::Unknown};
    QString m_deviceId;
    int m_cameraNumber{0};
};

class TaskDeviceBindings {
public:
    QList<TaskDeviceBinding> bindings() const { return m_bindings; }

    QString primaryPlcDeviceId() const
    {
        return deviceIdForRole(TaskDeviceRole::PrimaryPlc);
    }

    void setPrimaryPlcDeviceId(const QString &deviceId)
    {
        setDeviceIdForRole(TaskDeviceRole::PrimaryPlc, deviceId);
    }

    QString visionOutputDeviceId() const
    {
        return deviceIdForRole(TaskDeviceRole::VisionOutput);
    }

    void setVisionOutputDeviceId(const QString &deviceId)
    {
        setDeviceIdForRole(TaskDeviceRole::VisionOutput, deviceId);
    }

    QMap<int, QString> cameraNumberMap() const
    {
        QMap<int, QString> result;
        for (const auto &binding : m_bindings) {
            if (binding.role() == TaskDeviceRole::CameraNumber)
                result.insert(binding.cameraNumber(), binding.deviceId());
        }
        return result;
    }

    void setCameraNumberMap(const QMap<int, QString> &mapping)
    {
        for (int i = m_bindings.size() - 1; i >= 0; --i) {
            if (m_bindings.at(i).role() == TaskDeviceRole::CameraNumber)
                m_bindings.removeAt(i);
        }

        for (auto it = mapping.cbegin(); it != mapping.cend(); ++it) {
            if (!it.value().isEmpty())
                m_bindings.append(TaskDeviceBinding(TaskDeviceRole::CameraNumber,
                                                    it.value(),
                                                    it.key()));
        }
    }

    QString cameraDeviceId(int cameraNumber) const
    {
        for (const auto &binding : m_bindings) {
            if (binding.role() == TaskDeviceRole::CameraNumber &&
                binding.cameraNumber() == cameraNumber) {
                return binding.deviceId();
            }
        }
        return QString();
    }

    QJsonArray toJson() const
    {
        QJsonArray arr;
        for (const auto &binding : m_bindings)
            arr.append(binding.toJson());
        return arr;
    }

    bool fromJson(const QJsonValue &value)
    {
        m_bindings.clear();
        if (value.isUndefined() || value.isNull())
            return true;
        if (!value.isArray())
            return false;

        bool allOk = true;
        const QJsonArray arr = value.toArray();
        for (const QJsonValue &item : arr) {
            TaskDeviceBinding binding;
            if (!item.isObject() || !binding.fromJson(item.toObject())) {
                allOk = false;
                continue;
            }
            if (!binding.deviceId().isEmpty())
                m_bindings.append(binding);
        }
        return allOk;
    }

private:
    QString deviceIdForRole(TaskDeviceRole role) const
    {
        for (const auto &binding : m_bindings) {
            if (binding.role() == role)
                return binding.deviceId();
        }
        return QString();
    }

    void setDeviceIdForRole(TaskDeviceRole role, const QString &deviceId)
    {
        for (int i = m_bindings.size() - 1; i >= 0; --i) {
            if (m_bindings.at(i).role() == role)
                m_bindings.removeAt(i);
        }
        if (!deviceId.isEmpty())
            m_bindings.append(TaskDeviceBinding(role, deviceId));
    }

    QList<TaskDeviceBinding> m_bindings;
};

} // namespace vc::model

#endif // TASK_DEVICE_BINDING_H
