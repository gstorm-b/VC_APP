#ifndef TASK_DEFINE_H
#define TASK_DEFINE_H


#include "utils/meta_utils.h"

namespace vc::model {
Q_NAMESPACE

enum class CameraSourceType {
    Source_Owned,
    Source_Shared
};
Q_ENUM_NS(CameraSourceType)

enum class TaskType{
    UndefineTask,
    LocalizationTask,
    PickPlaceTask,
    InspectTask
};
Q_ENUM_NS(TaskType)

// only use for lingust
static inline const char* enum_keys_task_defines[] = {
    // CameraSourceType
    QT_TR_NOOP("Source_Owned"),
    QT_TR_NOOP("Source_Shared"),

    // TaskType
    QT_TR_NOOP("UndefineTask"),
    QT_TR_NOOP("LocalizationTask"),
    QT_TR_NOOP("PickPlaceTask"),
    QT_TR_NOOP("InspectTask"),
};

[[maybe_unused]] static QString taskTypeToString(TaskType t) {
    return qenumToString(t);
};

[[maybe_unused]] static TaskType taskTypeFromString(QString t) {
    return stringToQEnum(t, TaskType::UndefineTask);
};




}


#endif // TASK_DEFINE_H
