#include "task_factory.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QMutexLocker>
#include <QDebug>

#include "model/task_localization.h"

namespace vc::model {

ITask* TaskFactory::fromJson(const QJsonObject& obj,
                             QObject* parent) {

    if (!obj.contains("id") ||
        !obj.contains("name") ||
        !obj.contains("taskType") ||
        !obj.contains("taskConfig")) {

        LOG_DEV_INFO << "Cannot convert task from json, wrong json format.";
        LOG_DEV_INFO << QJsonDocument(obj).toJson();
        return nullptr;
    }

    const QString type = obj["taskType"].toString();
    return TaskFactory::create(taskTypeFromString(type), obj, parent);
}

ITask* TaskFactory::create(const TaskType& type, const QJsonObject& obj, QObject* parent) {
    switch (type) {
    case vc::model::TaskType::LocalizationTask:
        return createTaskLocalization(obj, parent);
    default:
        return nullptr;
    }
    return nullptr;
}


ITask* TaskFactory::createTaskLocalization(const QJsonObject& obj, QObject* parent) {
    const QString taskId = obj["id"].toString();
    QString taskName = obj["name"].toString();
    if (taskId.isEmpty()) {
        return nullptr;
    }

    TaskLocalization* task = new TaskLocalization(taskName, taskId, parent);
    if (obj.contains("taskConfig") && obj["taskConfig"].isObject()) {
        TaskLocalizeConfig config;
        config.fromJson(obj["taskConfig"].toObject());
        task->setTaskLocalizeConfig(config);
    }
    return task;
}

} // namespace vc::model
