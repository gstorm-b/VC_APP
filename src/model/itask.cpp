#include "itask.h"
#include "model/project.h"
#include "device/device_manager.h"

namespace vc::model {

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
    syncRunnersWithDevices();
    m_taskRunner->enterCommission();
    emit commissionStarted();
}

void ITask::endCommission()
{
    m_taskRunner->enterIdle();
    emit commissionStopped();
}

void ITask::beginRuntime(bool mergeToTaskThread)
{
    syncRunnersWithDevices();
    m_taskRunner->enterRuntime(mergeToTaskThread);
    emit runtimeStarted();
}

void ITask::endRuntime()
{
    m_taskRunner->enterIdle();
    emit runtimeStopped();
}

void ITask::stopAll()
{
    m_taskRunner->enterIdle();
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
