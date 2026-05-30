#ifndef TASK_STATE_MACHINE_H
#define TASK_STATE_MACHINE_H

#include <QString>

#include "model/task_define.h"

namespace vc::model {

inline bool canTransitionTaskState(TaskState from, TaskState to)
{
    if (from == to) {
        return true;
    }

    switch (from) {
    case TaskState::Idle:
        return to == TaskState::CommissionStarting ||
               to == TaskState::RuntimeStarting ||
               to == TaskState::Faulted;

    case TaskState::CommissionStarting:
        return to == TaskState::Commission ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::Commission:
        return to == TaskState::RuntimeStarting ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::RuntimeStarting:
        return to == TaskState::Ready ||
               to == TaskState::Recovering ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::Ready:
        return to == TaskState::RunningCycle ||
               to == TaskState::Recovering ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::RunningCycle:
        return to == TaskState::Ready ||
               to == TaskState::Recovering ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::Recovering:
        return to == TaskState::Ready ||
               to == TaskState::RunningCycle ||
               to == TaskState::Stopping ||
               to == TaskState::Faulted;

    case TaskState::Faulted:
        return to == TaskState::Stopping;

    case TaskState::Stopping:
        return to == TaskState::Idle ||
               to == TaskState::Faulted;
    }

    return false;
}

inline QString buildTaskStateTransitionMessage(TaskState from,
                                               TaskState to,
                                               const QString &reason = QString())
{
    QString message = QStringLiteral("Task state transition: %1 -> %2")
                          .arg(taskStateToString(from), taskStateToString(to));
    if (!reason.trimmed().isEmpty()) {
        message += QStringLiteral(" (%1)").arg(reason.trimmed());
    }
    return message;
}

inline QString buildInvalidTaskStateTransitionMessage(TaskState from,
                                                      TaskState to,
                                                      const QString &reason = QString())
{
    QString message = QStringLiteral("Rejected task state transition: %1 -> %2")
                          .arg(taskStateToString(from), taskStateToString(to));
    if (!reason.trimmed().isEmpty()) {
        message += QStringLiteral(" (%1)").arg(reason.trimmed());
    }
    return message;
}

} // namespace vc::model

#endif // TASK_STATE_MACHINE_H
