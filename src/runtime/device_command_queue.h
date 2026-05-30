#ifndef DEVICE_COMMAND_QUEUE_H
#define DEVICE_COMMAND_QUEUE_H

#include <QList>
#include <QQueue>

#include "runtime/device_command.h"

namespace vc::runtime {

enum class DeviceCommandQueuePolicy {
    RejectWhenBusy,
    QueueWhenBusy,
    ReplacePendingSameKind,
};

struct DeviceCommandEnqueueDecision {
    bool accepted{false};
    bool shouldRunNow{false};
    bool replacedPending{false};
    int pendingCount{0};
    DeviceCommandResult rejection;
};

class DeviceCommandQueue {
public:
    explicit DeviceCommandQueue(int maxPending = 16)
        : m_maxPending(maxPending)
    {
    }

    DeviceCommandEnqueueDecision enqueue(const DeviceCommand &command,
                                         DeviceCommandQueuePolicy policy,
                                         bool busy)
    {
        DeviceCommandEnqueueDecision decision;
        if (!busy) {
            decision.accepted = true;
            decision.shouldRunNow = true;
            return decision;
        }

        if (policy == DeviceCommandQueuePolicy::RejectWhenBusy) {
            decision.rejection = DeviceCommandResult::rejected(
                command,
                DeviceCommandResultCode::Busy,
                QStringLiteral("Runner is busy with another command."));
            return decision;
        }

        if (policy == DeviceCommandQueuePolicy::ReplacePendingSameKind) {
            QList<DeviceCommand> rewritten;
            rewritten.reserve(m_pending.size());
            bool replaced = false;
            for (const DeviceCommand &pending : m_pending) {
                if (!replaced && pending.kind == command.kind) {
                    replaced = true;
                    continue;
                }
                rewritten.push_back(pending);
            }
            m_pending.clear();
            for (const DeviceCommand &item : rewritten) {
                m_pending.enqueue(item);
            }
            decision.replacedPending = replaced;
        }

        if (!decision.replacedPending && m_pending.size() >= m_maxPending) {
            decision.rejection = DeviceCommandResult::rejected(
                command,
                DeviceCommandResultCode::QueueFull,
                QStringLiteral("Command queue is full."));
            return decision;
        }

        m_pending.enqueue(command);
        decision.accepted = true;
        decision.pendingCount = m_pending.size();
        return decision;
    }

    bool hasPending() const
    {
        return !m_pending.isEmpty();
    }

    DeviceCommand takeNext()
    {
        return m_pending.dequeue();
    }

    void clear()
    {
        m_pending.clear();
    }

    int pendingCount() const
    {
        return m_pending.size();
    }

private:
    int m_maxPending{16};
    QQueue<DeviceCommand> m_pending;
};

} // namespace vc::runtime

#endif // DEVICE_COMMAND_QUEUE_H
