#ifndef TASK_FACTORY_H
#define TASK_FACTORY_H

#include "model/itask.h"

namespace vc::model {

class TaskFactory {

public:
    TaskFactory() = delete;
    ~TaskFactory() = delete;
    TaskFactory(const TaskFactory&) = delete;
    TaskFactory& operator=(const TaskFactory&) = delete;

    static ITask* fromJson(const QJsonObject& o,
                           QObject* parent = nullptr);

    static ITask* create(const TaskType& type,
                         const QJsonObject& obj,
                         QObject* parent = nullptr);

    static ITask* createTaskLocalization(const QJsonObject& obj,
                                         QObject* parent = nullptr);

};

}

#endif // TASK_FACTORY_H
