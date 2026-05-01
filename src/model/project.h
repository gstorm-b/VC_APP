#ifndef PROJECT_H
#define PROJECT_H

#include <QObject>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "model/itask.h"
#include "device/idevice.h"
#include "device/device_manager.h"
#include <memory>

namespace vc::model {

class Project : public QObject {
    Q_OBJECT

    Q_PROPERTY(QString name
                   READ name WRITE setName
                       NOTIFY nameChanged)
    Q_PROPERTY(QString version
                   READ version WRITE setVersion
                       NOTIFY versionChanged)
    Q_PROPERTY(QString createdAt
                   READ createdAt NOTIFY createdAtChanged)
    Q_PROPERTY(QString updatedAt
                   READ updatedAt NOTIFY updatedAtChanged)

public:
    explicit Project(QObject* parent = nullptr);
    ~Project();

    // ── Getters ────────────────────────────────────────
    QString name()      const { return m_name;      }
    QString author()    const { return m_author;    }
    QString description() const { return m_description; }
    QString version()   const { return m_version;   }
    QString createdAt() const { return m_createdAt; }
    QString updatedAt() const { return m_updatedAt; }

    // ── Setters ────────────────────────────────────────
    void setName(const QString& v);
    void setDescription(const QString &v);
    void setAuthor(const QString& v);
    void setVersion(const QString& v);
    void setCreatedAt(const QString& v);
    void setUpdatedAt(const QString& v);


    // ── Task management ────────────────────────────────
    /**
     * @brief addTask
     * @param task
     */
    bool addTask(ITask* task);
    bool removeTask(const QString& id);
    std::shared_ptr<vc::model::ITask> taskById(const QString& id) const;
    bool changeTaskName(const QString& id, const QString &name);
    bool isTaskNameOccupied(const QString& name) const;
    const QMap<QString, std::shared_ptr<vc::model::ITask>>& getCurrentTasks() const {
        return m_tasks;
    }

    // ── Device management ──────────────────────────────
    /**
     * @brief deviceManager
     * @return
     */
    std::shared_ptr<device::DeviceManager> deviceManager();
    std::shared_ptr<vc::device::IDevice> deviceById(const QString& id);

    // ── Serialize ──────────────────────────────────────
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &json);
    // QMap<QString, QMap<QString, cv::Mat>> toImageMaps() const;
    // bool fromImageMaps(const  QMap<QString, QMap<QString, cv::Mat>> &mapping);

    bool isValid() const {
        return !m_name.isEmpty() && !m_tasks.isEmpty();
    }

signals:
    void nameChanged();
    void authorChanged();
    void versionChanged();
    void createdAtChanged();
    void updatedAtChanged();
    void projectModificationOccurred();

    void taskCreated(QString id);
    void taskDeleted(QString id);
    void taskModified(QString id);
    void tasksChanged();

private:
    QString m_name;
    QString m_author;
    QString m_description;
    QString m_version   = "0.1";
    QString m_createdAt;
    QString m_updatedAt;

    std::shared_ptr<device::DeviceManager> m_dvManager;

    QMap<QString, std::shared_ptr<vc::model::ITask>> m_tasks;

    QSet<QString> m_occupieTaskdNames;
};

} // namespace vc::model

#endif // PROJECT_H
