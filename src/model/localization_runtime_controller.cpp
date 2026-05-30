#include "model/localization_runtime_controller.h"

#include <QTimer>

#include "logger/app_logger.h"
#include "model/task_localization.h"
#include "runtime/camera_runner.h"
#include "runtime/plc_runner.h"
#include "runtime/task_runner.h"
#include "runtime/vision_output_runner.h"

namespace vc::model {

namespace {

bool isHealthyStatus(vc::device::ConnectStatus status)
{
    return status == vc::device::ConnectStatus::Connected;
}

QString connectStatusName(vc::device::ConnectStatus status)
{
    switch (status) {
    case vc::device::ConnectStatus::NoConnection:
        return QStringLiteral("NoConnection");
    case vc::device::ConnectStatus::Disconnected:
        return QStringLiteral("Disconnected");
    case vc::device::ConnectStatus::Connected:
        return QStringLiteral("Connected");
    case vc::device::ConnectStatus::LostConnected:
        return QStringLiteral("LostConnected");
    case vc::device::ConnectStatus::ConnectFailed:
        return QStringLiteral("ConnectFailed");
    }

    return QStringLiteral("Unknown");
}

QString buildRecoveryProgressMessage(const LocalizationRecoveryPolicy &policy,
                                     const QString &deviceId,
                                     int retryCount,
                                     vc::device::ConnectStatus status)
{
    return QStringLiteral("Task runtime recovering: role=%1 deviceId=%2 status=%3 retries=%4/%5 intervalMs=%6")
        .arg(policy.roleName,
             deviceId,
             connectStatusName(status),
             QString::number(retryCount),
             QString::number(policy.maxRetries),
             QString::number(policy.retryIntervalMs));
}

QString buildRecoveryReadyMessage(const LocalizationRecoveryPolicy &policy,
                                  const QString &deviceId,
                                  int retryCount)
{
    return QStringLiteral("Task runtime ready: role=%1 deviceId=%2 retries=%3/%4")
        .arg(policy.roleName,
             deviceId,
             QString::number(retryCount),
             QString::number(policy.maxRetries));
}

} // namespace

LocalizationRuntimeController::LocalizationRuntimeController(QObject *parent)
    : QObject(parent)
{
}

void LocalizationRuntimeController::setPipeline(LocalizationPipeline *pipeline)
{
    m_pipeline = pipeline;
}

void LocalizationRuntimeController::configure(const TaskLocalizeConfig &config)
{
    m_config = config;
    m_signalMapper.configure(m_config);
}

void LocalizationRuntimeController::setActiveCameraNumber(int cameraNumber)
{
    m_activeCameraNumber = cameraNumber;
    if (m_task) {
        bindActiveCameraRole(m_task, m_activeCameraNumber);
    }
}

void LocalizationRuntimeController::setRecoveryPolicies(
    const LocalizationRecoveryPolicy &cameraPolicy,
    const LocalizationRecoveryPolicy &plcPolicy,
    const LocalizationRecoveryPolicy &visionOutputPolicy)
{
    m_cameraRecoveryPolicy = cameraPolicy;
    m_plcRecoveryPolicy = plcPolicy;
    m_visionOutputRecoveryPolicy = visionOutputPolicy;
}

LocalizationRuntimeController::SetupResult
LocalizationRuntimeController::setup(TaskLocalization *task)
{
    SetupResult result;
    m_valid = false;
    m_task = task;
    resetRuntimeBindings();

    if (!task) {
        result.errors.append(QStringLiteral("Task is null."));
        return result;
    }

    auto *taskRunner = task->taskRunner();
    if (!taskRunner) {
        result.errors.append(QStringLiteral("TaskRunner is null."));
        return result;
    }

    result.primaryPlcDeviceId = m_config.d->m_deviceBindings.primaryPlcDeviceId();
    result.visionOutputDeviceId = m_config.d->m_deviceBindings.visionOutputDeviceId();
    const QMap<int, QString> cameraMap = m_config.d->m_deviceBindings.cameraNumberMap();

    if (!validateDeviceRole(task,
                            result.primaryPlcDeviceId,
                            QStringLiteral("primary_plc"),
                            vc::device::DeviceType::PLC,
                            &result.errors)) {
        LOG_DEV_ERR << "LocalizationRuntimeController: invalid primary PLC role";
    }

    validateDeviceRole(task,
                       result.visionOutputDeviceId,
                       QStringLiteral("vision_output"),
                       vc::device::DeviceType::VisionOutput,
                       &result.errors);

    if (cameraMap.isEmpty()) {
        result.errors.append(QStringLiteral("Missing camera_number role binding."));
    } else {
        for (auto it = cameraMap.cbegin(); it != cameraMap.cend(); ++it) {
            validateDeviceRole(task,
                               it.value(),
                               QStringLiteral("camera_number_%1").arg(it.key()),
                               vc::device::DeviceType::Camera,
                               &result.errors);
        }
    }

    disconnect(m_plcValueConnection);
    if (!result.primaryPlcDeviceId.isEmpty() &&
        taskRunner->hasRunner(result.primaryPlcDeviceId)) {
        auto *plcRun = qobject_cast<vc::runtime::PlcRunner *>(
            taskRunner->runnerFor(result.primaryPlcDeviceId));
        if (plcRun) {
            m_plcValueConnection = connect(
                plcRun,
                &vc::runtime::PlcRunner::valueChanged,
                this,
                &LocalizationRuntimeController::handlePlcValues,
                Qt::UniqueConnection);
        } else {
            result.errors.append(QStringLiteral("primary_plc runner has wrong type."));
        }
    }

    bindFixedRoleRunners(task);
    if (m_activeCameraNumber < 0 && !cameraMap.isEmpty()) {
        m_activeCameraNumber = cameraMap.firstKey();
    }
    bindActiveCameraRole(task, m_activeCameraNumber);

    result.valid = result.errors.isEmpty();
    m_valid = result.valid;

    for (const QString &error : result.errors)
        LOG_DEV_ERR << "LocalizationRuntimeController setup:" << error;

    return result;
}

void LocalizationRuntimeController::execute()
{
    if (!m_valid) {
        LOG_DEV_ERR << "LocalizationRuntimeController::execute - runtime is not set up";
        return;
    }

    if (!m_pipeline) {
        LOG_DEV_ERR << "LocalizationRuntimeController::execute - pipeline is not set";
        return;
    }

    // Production cycle orchestration will call the pipeline once the trigger
    // and image-acquisition flow is formalized.
}

void LocalizationRuntimeController::handlePlcValues(const QMap<QString, QVariant> &values)
{
    if (values.isEmpty()) {
        LOG_USER_ERR << "Communication device passed an empty values map.";
        return;
    }

    const QList<LocalizationSignalEvent> events = m_signalMapper.mapValues(values);
    for (const LocalizationSignalEvent &event : events) {
        emit signalChanged(event.name, event.value);

        if (event.name == QStringLiteral("nActiveCamera")) {
            emit cameraNumberChanged(event.value);
            const int cameraNumber = event.value.toInt();
            if (cameraNumber > 0) {
                setActiveCameraNumber(cameraNumber);
            }
        } else if (event.name == QStringLiteral("nActivePattern")) {
            emit patternNumberChanged(event.value);
        }
    }
}

bool LocalizationRuntimeController::validateDeviceRole(TaskLocalization *task,
                                                       const QString &deviceId,
                                                       const QString &roleName,
                                                       vc::device::DeviceType expectedType,
                                                       QStringList *errors) const
{
    if (!errors)
        return false;

    if (deviceId.isEmpty()) {
        errors->append(QStringLiteral("Missing %1 role binding.").arg(roleName));
        return false;
    }

    if (!task->assignedDeviceIds().contains(deviceId)) {
        errors->append(QStringLiteral("%1 role device is not assigned to task: %2")
                           .arg(roleName, deviceId));
        return false;
    }

    auto *taskRunner = task->taskRunner();
    if (!taskRunner || !taskRunner->hasRunner(deviceId)) {
        errors->append(QStringLiteral("%1 role runner is not registered: %2")
                           .arg(roleName, deviceId));
        return false;
    }

    auto *runner = taskRunner->runnerFor(deviceId);
    if (!runner || !runner->device() || runner->device()->deviceType() != expectedType) {
        errors->append(QStringLiteral("%1 role runner has wrong device family: %2")
                           .arg(roleName, deviceId));
        return false;
    }

    return true;
}

void LocalizationRuntimeController::resetRuntimeBindings()
{
    disconnect(m_plcValueConnection);
    m_plcValueConnection = QMetaObject::Connection();

    clearRoleContext(RunnerRole::PrimaryPlc);
    clearRoleContext(RunnerRole::VisionOutput);
    clearRoleContext(RunnerRole::Camera);
}

void LocalizationRuntimeController::bindFixedRoleRunners(TaskLocalization *task)
{
    auto *taskRunner = task ? task->taskRunner() : nullptr;
    if (!taskRunner) {
        return;
    }

    const QString plcId = m_config.d->m_deviceBindings.primaryPlcDeviceId();
    if (taskRunner->hasRunner(plcId)) {
        bindRoleContext(RunnerRole::PrimaryPlc,
                        taskRunner->runnerFor(plcId),
                        plcId,
                        m_plcRecoveryPolicy);
    }

    const QString visionOutputId = m_config.d->m_deviceBindings.visionOutputDeviceId();
    if (taskRunner->hasRunner(visionOutputId)) {
        bindRoleContext(RunnerRole::VisionOutput,
                        taskRunner->runnerFor(visionOutputId),
                        visionOutputId,
                        m_visionOutputRecoveryPolicy);
    }
}

void LocalizationRuntimeController::bindActiveCameraRole(TaskLocalization *task, int cameraNumber)
{
    if (!task) {
        return;
    }

    const QString cameraId = m_config.d->m_deviceBindings.cameraDeviceId(cameraNumber);
    auto *taskRunner = task->taskRunner();
    if (!taskRunner || cameraId.isEmpty() || !taskRunner->hasRunner(cameraId)) {
        clearRoleContext(RunnerRole::Camera);
        return;
    }

    bindRoleContext(RunnerRole::Camera,
                    taskRunner->runnerFor(cameraId),
                    cameraId,
                    m_cameraRecoveryPolicy);
}

void LocalizationRuntimeController::bindRoleContext(
    RunnerRole role,
    vc::runtime::IDeviceRunner *runner,
    const QString &deviceId,
    const LocalizationRecoveryPolicy &policy)
{
    clearRoleContext(role);

    if (!runner) {
        return;
    }

    RoleRecoveryContext context;
    context.roleName = policy.roleName;
    context.deviceId = deviceId;
    context.runner = runner;
    context.policy = policy;

    m_recoveryContexts.insert(roleKey(role), context);

    switch (role) {
    case RunnerRole::PrimaryPlc:
        connect(runner, &vc::runtime::IDeviceRunner::connectStatusChanged,
                this, &LocalizationRuntimeController::onPrimaryPlcStatusChanged,
                Qt::UniqueConnection);
        connect(runner, &vc::runtime::IDeviceRunner::errorOccurred,
                this, &LocalizationRuntimeController::onPrimaryPlcError,
                Qt::UniqueConnection);
        break;
    case RunnerRole::VisionOutput:
        connect(runner, &vc::runtime::IDeviceRunner::connectStatusChanged,
                this, &LocalizationRuntimeController::onVisionOutputStatusChanged,
                Qt::UniqueConnection);
        connect(runner, &vc::runtime::IDeviceRunner::errorOccurred,
                this, &LocalizationRuntimeController::onVisionOutputError,
                Qt::UniqueConnection);
        break;
    case RunnerRole::Camera:
        connect(runner, &vc::runtime::IDeviceRunner::connectStatusChanged,
                this, &LocalizationRuntimeController::onCameraStatusChanged,
                Qt::UniqueConnection);
        connect(runner, &vc::runtime::IDeviceRunner::errorOccurred,
                this, &LocalizationRuntimeController::onCameraError,
                Qt::UniqueConnection);
        break;
    }
}

void LocalizationRuntimeController::clearRoleContext(RunnerRole role)
{
    const int key = roleKey(role);
    if (!m_recoveryContexts.contains(key)) {
        return;
    }

    const RoleRecoveryContext context = m_recoveryContexts.value(key);
    if (context.runner) {
        disconnect(context.runner, nullptr, this, nullptr);
    }
    m_recoveryContexts.remove(key);
}

void LocalizationRuntimeController::handleRoleStatusChanged(RunnerRole role,
                                                            vc::device::ConnectStatus status)
{
    if (!m_task || !m_task->taskRunner() ||
        m_task->taskRunner()->currentPhase() != vc::runtime::TaskRunner::Phase::Runtime) {
        return;
    }

    const int key = roleKey(role);
    if (!m_recoveryContexts.contains(key)) {
        return;
    }

    RoleRecoveryContext context = m_recoveryContexts.value(key);
    if (!context.runner) {
        m_recoveryContexts.remove(key);
        return;
    }

    if (isHealthyStatus(status)) {
        const int retryHistory = context.retryCount;
        const bool wasRecovering = retryHistory > 0 || context.retryScheduled;
        context.retryCount = 0;
        context.retryScheduled = false;
        context.faultRaised = false;
        m_recoveryContexts.insert(key, context);
        if (wasRecovering) {
            emit runtimeReady(buildRecoveryReadyMessage(
                context.policy,
                context.deviceId,
                retryHistory));
        }
        return;
    }

    const LocalizationRecoveryAction action = decideRecoveryAction(
        context.policy,
        status,
        context.retryCount,
        context.retryScheduled);

    if (action == LocalizationRecoveryAction::RetryScheduled) {
        context.retryScheduled = true;
        context.retryCount += 1;
        m_recoveryContexts.insert(key, context);
        emit runtimeRecovering(buildRecoveryProgressMessage(
            context.policy,
            context.deviceId,
            context.retryCount,
            status));
        scheduleRoleReconnect(role);
        return;
    }

    if (action == LocalizationRecoveryAction::EscalateFault) {
        context.faultRaised = true;
        m_recoveryContexts.insert(key, context);
        raiseRoleFault(role, status);
    }
}

void LocalizationRuntimeController::scheduleRoleReconnect(RunnerRole role)
{
    const int key = roleKey(role);
    if (!m_recoveryContexts.contains(key)) {
        return;
    }

    const RoleRecoveryContext context = m_recoveryContexts.value(key);
    if (!context.runner) {
        return;
    }

    LOG_USER_WARN << "Recovery retry scheduled."
                  << "role=" << context.policy.roleName
                  << "deviceId=" << context.deviceId
                  << "attempt=" << context.retryCount
                  << "max=" << context.policy.maxRetries
                  << "intervalMs=" << context.policy.retryIntervalMs;

    QTimer::singleShot(context.policy.retryIntervalMs, this, [this, role]() {
        const int currentKey = roleKey(role);
        if (!m_recoveryContexts.contains(currentKey)) {
            return;
        }
        RoleRecoveryContext current = m_recoveryContexts.value(currentKey);
        current.retryScheduled = false;
        m_recoveryContexts.insert(currentKey, current);
        requestRoleConnectNow(role);
    });
}

void LocalizationRuntimeController::requestRoleConnectNow(RunnerRole role)
{
    const int key = roleKey(role);
    if (!m_recoveryContexts.contains(key)) {
        return;
    }

    RoleRecoveryContext context = m_recoveryContexts.value(key);
    if (!context.runner) {
        m_recoveryContexts.remove(key);
        return;
    }

    switch (role) {
    case RunnerRole::Camera: {
        auto *cameraRunner = qobject_cast<vc::runtime::CameraRunner *>(context.runner.data());
        if (cameraRunner) {
            cameraRunner->requestConnect();
        }
        break;
    }
    case RunnerRole::PrimaryPlc: {
        auto *plcRunner = qobject_cast<vc::runtime::PlcRunner *>(context.runner.data());
        if (plcRunner) {
            plcRunner->requestConnect();
        }
        break;
    }
    case RunnerRole::VisionOutput: {
        auto *visionRunner = qobject_cast<vc::runtime::VisionOutputRunner *>(context.runner.data());
        if (visionRunner) {
            visionRunner->requestConnect();
        }
        break;
    }
    }

    LOG_USER_WARN << "Recovery reconnect requested."
                  << "role=" << context.policy.roleName
                  << "deviceId=" << context.deviceId
                  << "attempt=" << context.retryCount
                  << "max=" << context.policy.maxRetries;
}

void LocalizationRuntimeController::raiseRoleFault(RunnerRole role,
                                                   vc::device::ConnectStatus status)
{
    const int key = roleKey(role);
    if (!m_recoveryContexts.contains(key)) {
        return;
    }

    const RoleRecoveryContext context = m_recoveryContexts.value(key);
    const QString message = buildRecoveryFaultMessage(
        context.policy,
        context.deviceId,
        status,
        context.retryCount);

    LOG_USER_ERR << message;
    emit runtimeFault(message);
}

void LocalizationRuntimeController::onPrimaryPlcStatusChanged(vc::device::ConnectStatus status)
{
    handleRoleStatusChanged(RunnerRole::PrimaryPlc, status);
}

void LocalizationRuntimeController::onVisionOutputStatusChanged(vc::device::ConnectStatus status)
{
    handleRoleStatusChanged(RunnerRole::VisionOutput, status);
}

void LocalizationRuntimeController::onCameraStatusChanged(vc::device::ConnectStatus status)
{
    handleRoleStatusChanged(RunnerRole::Camera, status);
}

void LocalizationRuntimeController::onPrimaryPlcError(const QString &message)
{
    LOG_USER_WARN << "primary_plc runtime error:" << message;
}

void LocalizationRuntimeController::onVisionOutputError(const QString &message)
{
    LOG_USER_WARN << "vision_output runtime error:" << message;
}

void LocalizationRuntimeController::onCameraError(const QString &message)
{
    LOG_USER_WARN << "camera runtime error:" << message;
}

} // namespace vc::model
