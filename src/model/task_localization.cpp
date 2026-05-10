#include "task_localization.h"

namespace vc::model {

TaskLocalization::TaskLocalization(QString name, QString id, QObject* parent)
    : ITask(id, parent), m_config()
{
    this->setName(name);

    this->blockSignals(true);
    this->setTaskConfig(&m_config);
    this->blockSignals(false);

    m_patternManager = new mtc::PatternGroupManager(this);
}

// ── Typed runner helpers ──────────────────────────────────────────────────────

vc::runtime::CameraRunner *TaskLocalization::cameraRunner(const QString &deviceId) const
{
    return qobject_cast<vc::runtime::CameraRunner *>(
        taskRunner()->runnerFor(deviceId));
}

vc::runtime::McDeviceRunner *TaskLocalization::mcRunner(const QString &deviceId) const
{
    return qobject_cast<vc::runtime::McDeviceRunner *>(
        taskRunner()->runnerFor(deviceId));
}

vc::device::CameraDevice *TaskLocalization::camera() const
{
    if (m_cameraDeviceId.isEmpty()) return nullptr;
    auto *runner = cameraRunner(m_cameraDeviceId);
    return runner ? runner->typedDevice() : nullptr;
}

vc::device::McProtocolDevice *TaskLocalization::mcDevice() const
{
    if (m_mcDeviceId.isEmpty()) return nullptr;
    auto *runner = mcRunner(m_mcDeviceId);
    return runner ? runner->typedDevice() : nullptr;
}

// ── Task lifecycle ────────────────────────────────────────────────────────────

void TaskLocalization::setupTask()
{
    // Called after commission: device roles are already assigned via
    // setCameraDeviceId() / setMcDeviceId().  Just verify runners exist.

    if (!m_cameraDeviceId.isEmpty() && !taskRunner()->hasRunner(m_cameraDeviceId)) {
        LOG_DEV_ERR << "TaskLocalization::setupTask – camera runner not registered";
    }

    if (!m_mcDeviceId.isEmpty() && !taskRunner()->hasRunner(m_mcDeviceId)) {
        LOG_DEV_ERR << "TaskLocalization::setupTask – MC device runner not registered";
    }

    m_isValid = (!m_cameraDeviceId.isEmpty() && taskRunner()->hasRunner(m_cameraDeviceId));
}

void TaskLocalization::executeLocalization()
{
    // This method runs on the task runtime thread (m_taskRunner->runtimeThread()).
    // Interact with devices via their runners using QueuedConnection signals.

    if (!m_isValid) {
        LOG_DEV_ERR << "TaskLocalization::executeLocalization – task not set up";
        return;
    }

    // Example: trigger a grab
    // auto *camRunner = cameraRunner(m_cameraDeviceId);
    // if (camRunner) camRunner->requestSingleShot();
    // → onGrabFinished() arrives back when done
}

void TaskLocalization::onCameraNumberChanged()
{
}

void TaskLocalization::onPatternNumberChanged()
{
}

} // namespace vc::model
