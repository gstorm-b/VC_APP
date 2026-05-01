#ifndef ITASK_CONFIG_H
#define ITASK_CONFIG_H

#include <QObject>
#include <QJsonObject>
#include "model/task_define.h"

namespace vc::model {

class ITaskConfig  {
public:
    virtual ~ITaskConfig() = default;

    virtual const QMetaObject &getMetaObject() const = 0;
    virtual TaskType taskType() const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual bool fromJson(const QJsonObject& obj) = 0;
    virtual ITaskConfig* copy() = 0;

};

}

#endif // ITASK_CONFIG_H
