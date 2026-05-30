#include "itask.h"
#include "model/project.h"
#include "device/device_manager.h"

namespace vc::model {

bool ITask::transitionTaskState(TaskState targetState, const QString &reason)
{
    if (m_taskState == targetState) {
        return true;
    }

    if (!canTransitionTaskState(m_taskState, targetState)) {
        LOG_USER_WARN << buildInvalidTaskStateTransitionMessage(
            m_taskState, targetState, reason);
        return false;
    }

    LOG_USER_INFO << buildTaskStateTransitionMessage(
        m_taskState, targetState, reason);
    m_taskState = targetState;
    emit taskStateChanged(m_taskState);
    return true;
}

// ── Assigned-device helpers ───────────────────────────────────────────────────

std::shared_ptr<vc::device::IDevice>
ITask::getTaskDevice(const QString &deviceId) const {
    std::shared_ptr<vc::device::IDevice> result;
    if (!m_proj) return result;


    auto dm = m_proj->deviceManager();
    if (!dm) return result;

    result = m_proj->deviceManager()->deviceById(deviceId);
    return result;
}

QList<std::shared_ptr<vc::device::IDevice>>
ITask::assignedDevicesOfType(vc::device::DeviceType t) const {
    QList<std::shared_ptr<vc::device::IDevice>> result;
    if (!m_proj) return result;

    auto dm = m_proj->deviceManager();
    if (!dm) return result;

    for (const QString &id : m_assignedDeviceIds) {
        auto dev = dm->deviceById(id);
        if (dev && dev->deviceType() == t)
            result.append(dev);
    }
    return result;
}

// ── Phase management ──────────────────────────────────────────────────────────

void ITask::beginCommission()
{
    if (!transitionTaskState(TaskState::CommissionStarting,
                             QStringLiteral("beginCommission"))) {
        return;
    }

    syncRunnersWithDevices();
    m_taskRunner->enterCommission();
    transitionTaskState(TaskState::Commission,
                        QStringLiteral("commission resources ready"));
    emit commissionStarted();
}

void ITask::endCommission()
{
    if (!transitionTaskState(TaskState::Stopping,
                             QStringLiteral("endCommission"))) {
        return;
    }

    m_taskRunner->enterIdle();
    transitionTaskState(TaskState::Idle,
                        QStringLiteral("commission stopped"));
    emit commissionStopped();
}

void ITask::beginRuntime(bool mergeToTaskThread)
{
    if (!transitionTaskState(TaskState::RuntimeStarting,
                             QStringLiteral("beginRuntime"))) {
        return;
    }

    syncRunnersWithDevices();
    m_taskRunner->enterRuntime(mergeToTaskThread);
    transitionTaskState(TaskState::Ready,
                        QStringLiteral("runtime ready"));
    emit runtimeStarted();
}

void ITask::endRuntime()
{
    if (!transitionTaskState(TaskState::Stopping,
                             QStringLiteral("endRuntime"))) {
        return;
    }

    m_taskRunner->enterIdle();
    transitionTaskState(TaskState::Idle,
                        QStringLiteral("runtime stopped"));
    emit runtimeStopped();
}

void ITask::stopAll()
{
    if (m_taskState != TaskState::Idle &&
        !transitionTaskState(TaskState::Stopping,
                             QStringLiteral("stopAll"))) {
        return;
    }

    m_taskRunner->enterIdle();
    transitionTaskState(TaskState::Idle,
                        QStringLiteral("all task activity stopped"));
}

// ── Runner sync ───────────────────────────────────────────────────────────────

void ITask::syncRunnersWithDevices()
{
    if (!m_proj) return;

    auto dm = m_proj->deviceManager();
    if (!dm) return;

    // 1. Register runners for newly assigned devices.
    //    TaskRunner::registerDevice() auto-starts + attaches the runner
    //    if we're already past Idle (Commission / Runtime), so the new
    //    device is immediately ready for the Widget to drive.
    for (const QString &id : m_assignedDeviceIds) {
        if (!m_taskRunner->hasRunner(id)) {
            auto device = dm->deviceById(id);
            if (device) {
                m_taskRunner->registerDevice(id, device);
            }
        }
    }

    // 2. Unregister runners for devices that are no longer assigned.
    //    (User clicked "remove device" → free its thread, drop the runner.)
    for (const QString &id : m_taskRunner->registeredDeviceIds()) {
        if (!m_assignedDeviceIds.contains(id)) {
            m_taskRunner->unregisterDevice(id);
        }
    }
}

} // namespace vc::model
