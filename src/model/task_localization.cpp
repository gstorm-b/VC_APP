#include "task_localization.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QPointer>
#include <QRegularExpression>
#include <QThread>

#include "device/output_device/vision_output_config.h"
#include "matching/match_group.h"
#include "matching/match_pattern.h"
#include "model/project.h"
#include "runtime/vision_output_runner.h"

namespace vc::model {

namespace {

// ── Image-blob key ────────────────────────────────────────────────────────────
// Stable per-pattern key for the project_images table.  Pattern *names* may
// be renamed by the user without going through a "rename in storage" step,
// so we key on the stable (groupNumber, patternNumber) pair instead.
inline QString imageKey(int groupNumber, int patternNumber) {
    return QStringLiteral("g%1_p%2").arg(groupNumber).arg(patternNumber);
}

inline bool parseImageKey(const QString &key, int &groupNumber, int &patternNumber) {
    // Expected form: "g{int}_p{int}"
    static const QRegularExpression re(QStringLiteral("^g(-?\\d+)_p(-?\\d+)$"));
    const auto m = re.match(key);
    if (!m.hasMatch()) return false;
    bool ok1 = false, ok2 = false;
    groupNumber   = m.captured(1).toInt(&ok1);
    patternNumber = m.captured(2).toInt(&ok2);
    return ok1 && ok2;
}

} // anonymous namespace



TaskLocalization::TaskLocalization(QString name, QString id, QObject* parent)
    : ITask(id, parent), m_config()
{
    this->setName(name);

    this->blockSignals(true);
    this->setTaskConfig(&m_config);
    this->blockSignals(false);

    m_limitDeviceMap.insert(device::DeviceType::PLC, kLimitCommDevice);
    m_limitDeviceMap.insert(device::DeviceType::VisionOutput, kLimitVisionOutputDevice);
    m_limitDeviceMap.insert(device::DeviceType::Camera, kLimitNumCamera);

    m_patternManager = new mtc::PatternGroupManager(this);
    qRegisterMetaType<mtc::MatchResult>("mtc::MatchResult");
    qRegisterMetaType<cv::Mat>("cv::Mat");
    qRegisterMetaType<CameraWorkspace>("CameraWorkspace");
    qRegisterMetaType<CameraWorkspace>("vc::model::CameraWorkspace");
    qRegisterMetaType<std::shared_ptr<mtc::MatchGroup>>("std::shared_ptr<mtc::MatchGroup>");
    qRegisterMetaType<std::shared_ptr<mtc::IRobotPickingChecker>>(
        "std::shared_ptr<mtc::IRobotPickingChecker>");

    matchingRunner = new QThread();
    matchingRunner->setObjectName(QStringLiteral("LocalizationMatchingThread"));
    wireMatchingWorkerSignals();
    matchingRunner->start();
    createRuntimeController();
}

TaskLocalization::~TaskLocalization()
{
    destroyRuntimeController();
    if (matchingRunner) {
        matchingRunner->quit();
        if (!matchingRunner->wait(3000)) {
            LOG_USER_WARN << "TaskLocalization matching worker did not stop within timeout.";
        }
        delete matchingRunner;
        matchingRunner = nullptr;
    }
}

void TaskLocalization::setTaskLocalizeConfig(const TaskLocalizeConfig &cfg)
{
    m_config = cfg;
    this->setTaskConfig(&m_config);
    queueConfigureRuntimeController();
}

bool TaskLocalization::isReachLimitOfDeviceType(vc::device::DeviceType t) const {

    QList<std::shared_ptr<vc::device::IDevice>> result;
    if (!m_proj) return true;

    auto dm = m_proj->deviceManager();
    if (!dm) return true;

    auto assigned_device_ids = this->assignedDeviceIds();
    int device_counter = 0;
    int limit = m_limitDeviceMap.value(t);

    for (const QString &id : assigned_device_ids) {
        auto dev = dm->deviceById(id);
        if (dev && dev->deviceType() == t) {
            device_counter++;
        }

        if (device_counter >= limit) {
            return true;
        }
    }

    return false;
}

void TaskLocalization::beginRuntime(bool mergeToTaskThread)
{
    if (mergeToTaskThread) {
        LOG_USER_WARN << "Localization runtime keeps per-device threads; mergeToTaskThread ignored.";
    }

    if (!transitionTaskState(TaskState::RuntimeStarting,
                             QStringLiteral("beginRuntime"))) {
        return;
    }

    syncRunnersWithDevices();
    taskRunner()->enterRuntime(false);
    createRuntimeController();
    if (auto *thread = taskRunner()->runtimeThread()) {
        if (m_runtimeController->thread() != thread) {
            m_runtimeController->moveToThread(thread);
        }
    }

    setupTask();
    if (!m_runtimeController || !m_runtimeController->isValid()) {
        transitionTaskState(TaskState::Faulted,
                            QStringLiteral("Runtime start aborted: setupTask failed"));
        return;
    }

    emit runtimeStarted();
}

void TaskLocalization::endRuntime()
{
    destroyRuntimeController();
    ITask::endRuntime();
    createRuntimeController();
}

void TaskLocalization::stopAll()
{
    destroyRuntimeController();
    ITask::stopAll();
    createRuntimeController();
}

// ── Typed runner helpers ──────────────────────────────────────────────────────

vc::runtime::CameraRunner *TaskLocalization::cameraRunner(const QString &deviceId) const
{
    if (!taskRunner()) return nullptr;
    return qobject_cast<vc::runtime::CameraRunner *>(
        taskRunner()->runnerFor(deviceId));
}

vc::runtime::PlcRunner *TaskLocalization::plcRunner(const QString &deviceId) const
{
    if (!taskRunner()) return nullptr;
    return qobject_cast<vc::runtime::PlcRunner *>(
        taskRunner()->runnerFor(deviceId));
}

vc::device::PlcDevice *TaskLocalization::plcDevice() const
{
    if (m_plcDeviceId.isEmpty()) return nullptr;
    auto *runner = plcRunner(m_plcDeviceId);
    return runner ? runner->typedDevice() : nullptr;
}

QJsonObject TaskLocalization::toJson() const {
    QJsonObject obj = ITask::toJson();

    // Pattern library — fully delegated to the manager.  Training images
    // are excluded from JSON and travel through the project_images BLOB
    // table (see getTaskImageMap() / loadTaskImageMap()).
    if (m_patternManager)
        obj["patternManager"] = m_patternManager->toJson();

    return obj;
}

bool TaskLocalization::fromJson(const QJsonObject& obj) {
    bool isOk = ITask::fromJson(obj);
    if (!isOk) return false;

    // Rebuild the pattern library.  Pattern images are restored later
    // by loadTaskImageMap().
    if (m_patternManager) {
        if (!m_patternManager->fromJson(obj["patternManager"].toObject()))
            isOk = false;
    }
    return isOk;
}

// ── Image BLOB I/O ───────────────────────────────────────────────────────────

QMap<QString, cv::Mat> TaskLocalization::getTaskImageMap() {
    QMap<QString, cv::Mat> map;

    if (m_patternManager) {
        for (const auto &group : m_patternManager->groups()) {
            if (!group) continue;
            const int gn = group->number();
            for (const auto &pattern : group->patterns()) {
                if (!pattern) continue;
                const cv::Mat &img = pattern->config().m_rawImage;
                if (img.empty()) continue;   // skip patterns without a training image
                map.insert(imageKey(gn, pattern->number()), img);
            }
        }
    }

    // Camera workspace reference images (key form "ws_{cameraId}").
    const QMap<QString, cv::Mat> wsImages = m_config.cameraWorkspaces().getImageMap();
    for (auto it = wsImages.cbegin(); it != wsImages.cend(); ++it)
        map.insert(it.key(), it.value());

    return map;
}

bool TaskLocalization::loadTaskImageMap(QMap<QString, cv::Mat> &mapping) {
    if (!m_patternManager) return false;
    if (mapping.isEmpty())  return true;   // nothing to inject

    bool allOk = true;
    for (auto it = mapping.cbegin(); it != mapping.cend(); ++it) {
        // Camera workspace reference image (key form "ws_{cameraId}").
        QString workspaceCameraId;
        if (CameraWorkspaceMap::parseImageKey(it.key(), workspaceCameraId)) {
            m_config.d->m_cameraWorkspaces.setReferenceImage(workspaceCameraId, it.value());
            continue;
        }

        int gn = 0, pn = 0;
        if (!parseImageKey(it.key(), gn, pn)) {
            LOG_DEV_ERR << "TaskLocalization::loadTaskImageMap – bad key:" << it.key();
            allOk = false;
            continue;
        }

        auto group = m_patternManager->findGroupByNumber(gn);
        if (!group) {
            LOG_DEV_ERR << "TaskLocalization::loadTaskImageMap – group" << gn
                        << "not found for key" << it.key();
            allOk = false;
            continue;
        }

        auto *pattern = group->findPatternByNumber(pn);
        if (!pattern) {
            LOG_DEV_ERR << "TaskLocalization::loadTaskImageMap – pattern" << pn
                        << "not found in group" << gn;
            allOk = false;
            continue;
        }

        const QString groupName   = QString::fromStdWString(group->name());
        const QString patternName = QString::fromStdWString(pattern->name());
        if (auto r = m_patternManager->setPatternImage(groupName, patternName, it.value()); !r) {
            LOG_DEV_ERR << "TaskLocalization::loadTaskImageMap – setPatternImage failed:"
                        << QString::fromStdWString(r.error);
            allOk = false;
        }
    }
    return allOk;
}

// ── Task lifecycle ────────────────────────────────────────────────────────────

void TaskLocalization::setupTask()
{
    if (!m_runtimeController) {
        m_isValid = false;
        return;
    }

    const auto result = setupRuntimeController();
    m_plcDeviceId = result.primaryPlcDeviceId;
    m_isValid = result.valid;
}

void TaskLocalization::executeLocalization()
{
    if (!m_runtimeController || !m_runtimeController->isValid()) {
        LOG_DEV_ERR << "TaskLocalization::executeLocalization – task not set up";
        return;
    }

    if (taskState() != TaskState::Ready &&
        taskState() != TaskState::RunningCycle) {
        LOG_USER_WARN << buildInvalidTaskStateTransitionMessage(
            taskState(),
            TaskState::RunningCycle,
            QStringLiteral("executeLocalization"));
        return;
    }

    auto *controller = m_runtimeController;
    if (controller) {
        QMetaObject::invokeMethod(controller, [controller]() {
            controller->execute();
        }, Qt::QueuedConnection);
    }
}

void TaskLocalization::setCameraNumber(int number) {
    if (!taskRunner()) {
        return;
    }

    if (taskRunner()->currentPhase() != runtime::TaskRunner::Phase::Runtime) {
        return;
    }

    QString cam_id = m_config.d->m_deviceBindings.cameraDeviceId(number);
    if (cam_id.isEmpty()) {
        LOG_USER_WARN << tr("Cannot change camera, not found camera number %1").arg(number);
        return;
    }

    if (!assignedDeviceIds().contains(cam_id)) {
        LOG_USER_WARN << tr("Cannot change camera, not found camera number %1").arg(number);
        return;
    }

    std::shared_ptr<device::IDevice> device = getTaskDevice(cam_id);
    if (!device || device->deviceType() != device::DeviceType::Camera) {
        LOG_USER_WARN << tr("Cannot change camera, device %1 with id %2 isn't camera type")
                             .arg(number).arg(cam_id);
        return;
    }

    queueSetActiveCameraNumber(number);
}

void TaskLocalization::setPatternNumber(int number) {
    queueSetActivePatternGroupNumber(number);
}

void TaskLocalization::onCommDeviceValueChanged(QMap<QString, QVariant> values) {
    queueHandlePlcValues(values);
}

void TaskLocalization::onSignalChangeCameraNumber(QVariant value) {
    bool is_ok = false;
    int number = value.toInt(&is_ok);

    if (!is_ok) {
        LOG_USER_WARN << tr("Task %1 change camera number failed, value %2")
                             .arg(name()).arg(value.toChar());
    }

    setCameraNumber(number);
}


void TaskLocalization::onSignalChangePatternNumber(QVariant value) {
    bool is_ok = false;
    int number = value.toInt(&is_ok);

    if (!is_ok) {
        LOG_USER_WARN << tr("Task %1 change pattern number failed, value %2")
        .arg(name()).arg(value.toChar());
    }

    setPatternNumber(number);
}

void TaskLocalization::onRuntimeCycleStarted(const QString &message)
{
    transitionTaskState(TaskState::RunningCycle, message);
}

void TaskLocalization::onRuntimeRecovering(const QString &message)
{
    transitionTaskState(TaskState::Recovering, message);
}

void TaskLocalization::onRuntimeReady(const QString &message)
{
    if (taskState() == TaskState::Faulted ||
        taskState() == TaskState::Stopping ||
        taskState() == TaskState::Idle) {
        return;
    }

    transitionTaskState(TaskState::Ready, message);
}

void TaskLocalization::onRuntimeFault(const QString &message)
{
    transitionTaskState(TaskState::Faulted, message);
}

void TaskLocalization::createRuntimeController()
{
    if (m_runtimeController) {
        return;
    }

    m_runtimeController = new LocalizationRuntimeController();
    m_runtimeController->configure(m_config);
    wireRuntimeControllerSignals();
}

void TaskLocalization::destroyRuntimeController()
{
    if (!m_runtimeController) {
        return;
    }

    auto *controller = m_runtimeController;
    m_runtimeController = nullptr;
    controller->disconnect(this);
    disconnect(controller, nullptr, this, nullptr);

    if (controller->thread() && controller->thread() != QThread::currentThread()
        && controller->thread()->isRunning()) {
        QMetaObject::invokeMethod(controller, [controller]() {
            controller->deleteLater();
        }, Qt::QueuedConnection);
    } else {
        delete controller;
    }
}

void TaskLocalization::wireRuntimeControllerSignals()
{
    if (!m_runtimeController) {
        return;
    }

    connect(m_runtimeController, &LocalizationRuntimeController::signalChanged,
            this, &ITask::signalChanged, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::cycleResultUpdated,
            this, &TaskLocalization::cycleResultUpdated, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::taskLogAppended,
            this, &TaskLocalization::taskLogAppended, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeCycleStarted,
            this, &TaskLocalization::onRuntimeCycleStarted, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeRecovering,
            this, &TaskLocalization::onRuntimeRecovering, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeReady,
            this, &TaskLocalization::onRuntimeReady, Qt::QueuedConnection);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeFault,
            this, &TaskLocalization::onRuntimeFault, Qt::QueuedConnection);

    if (m_matchingWorker) {
        QPointer<LocalizationRuntimeController> controller(m_runtimeController);
        connect(m_runtimeController,
                &LocalizationRuntimeController::runtimeMatchingRequested,
                m_matchingWorker,
                [this, controller](int cycleId,
                                   std::shared_ptr<mtc::MatchGroup> group,
                                   CameraWorkspace workspace,
                                   cv::Mat image,
                                   std::shared_ptr<mtc::IRobotPickingChecker> pickingChecker) {

            // Persistent matcher: reused across cycles (runs on the
            // matchingRunner thread). The matcher + learned model are rebuilt
            // only when the active pattern group changes, never per cycle.
            if (!m_runtimeMatcher || m_loadedRuntimeGroup != group) {
                m_runtimeMatcher = std::make_unique<mtc::ImageMatcher>();
                m_loadedRuntimeGroup =
                    m_pipeline.loadModel(*m_runtimeMatcher, group) ? group : nullptr;
            }

            mtc::MatchResult result;
            if (m_loadedRuntimeGroup) {
                result = m_pipeline.runMatchOn(*m_runtimeMatcher, workspace, image,
                                               pickingChecker.get(),
                                               LocalizationPipeline::kRuntimeMaxObjects);
            }
            if (!controller) {
                return;
            }
            QMetaObject::invokeMethod(controller.data(),
                                      [controller, cycleId, result]() {
                if (controller) {
                    controller->onRuntimeMatchingFinished(cycleId, result);
                }
            }, Qt::QueuedConnection);
        }, Qt::QueuedConnection);
    }
}

std::shared_ptr<mtc::MatchGroup>
TaskLocalization::snapshotPatternGroup(const std::shared_ptr<mtc::MatchGroup> &source)
{
    if (!source) {
        return nullptr;
    }

    // Independent deep copy: own MatchGroupConfig (with cloned typeConfig) plus
    // an own pattern vector whose configs carry the training image (m_rawImage).
    // The worker can learn/match from this snapshot without touching — or racing
    // — the live PatternGroupManager group on the GUI thread.
    auto snapshot = std::make_shared<mtc::MatchGroup>();
    source->cloneConfigTo(*snapshot);
    for (const auto &pattern : source->patterns()) {
        if (pattern) {
            snapshot->addPattern(pattern->config());
        }
    }
    return snapshot;
}

LocalizationRuntimeController::RuntimeContext
TaskLocalization::buildRuntimeContext() const
{
    LocalizationRuntimeController::RuntimeContext context;
    context.config = m_config;
    context.primaryPlcDeviceId = m_config.d->m_deviceBindings.primaryPlcDeviceId();
    context.visionOutputDeviceId = m_config.d->m_deviceBindings.visionOutputDeviceId();
    context.primaryPlcRunner = plcRunner(context.primaryPlcDeviceId);
    context.visionOutputRunner = qobject_cast<vc::runtime::VisionOutputRunner *>(
        taskRunner() ? taskRunner()->runnerFor(context.visionOutputDeviceId) : nullptr);

    // Snapshot the robot kinematic check settings from the assigned vision
    // output device config (drives the per-object robotPossiblePickingCheck).
    if (auto visionDevice = getTaskDevice(context.visionOutputDeviceId)) {
        std::unique_ptr<device::IDeviceCfg> visionCfg(visionDevice->deviceConfig());
        if (auto *voutCfg = dynamic_cast<device::VisionOutputDeviceCfg *>(visionCfg.get())) {
            context.robotCheckConfig = voutCfg->m_kinematicCheck;
        }
    }

    context.cameraDeviceIds = m_config.d->m_deviceBindings.cameraNumberMap();

    for (auto it = context.cameraDeviceIds.cbegin(); it != context.cameraDeviceIds.cend(); ++it) {
        context.cameraRunners.insert(it.key(), cameraRunner(it.value()));

        auto device = getTaskDevice(it.value());
        auto camera = std::dynamic_pointer_cast<device::CameraDevice>(device);
        if (!camera) {
            continue;
        }
        std::unique_ptr<device::IDeviceCfg> cfg(camera->deviceConfig());
        auto *cameraCfg = dynamic_cast<device::CameraCfg *>(cfg.get());
        if (cameraCfg) {
            context.cameraCalibrators.insert(it.key(), cameraCfg->calibrator());
        }
    }

    if (!context.cameraDeviceIds.isEmpty()) {
        context.activeCameraNumber = context.cameraDeviceIds.firstKey();
    }

    if (m_patternManager) {
        const auto groups = m_patternManager->groups();
        for (const auto &source : groups) {
            if (auto snapshot = snapshotPatternGroup(source)) {
                context.patternGroups.insert(snapshot->number(), snapshot);
            }
        }
        if (!context.patternGroups.isEmpty()) {
            context.activePatternGroupNumber = context.patternGroups.firstKey();
        }
    }

    return context;
}

LocalizationRuntimeController::SetupResult TaskLocalization::setupRuntimeController()
{
    LocalizationRuntimeController::SetupResult result;
    auto *controller = m_runtimeController;
    if (!controller) {
        result.errors.append(QStringLiteral("Runtime controller is null."));
        return result;
    }

    const auto context = buildRuntimeContext();
    if (controller->thread() == QThread::currentThread()) {
        return controller->setup(context);
    }

    QMetaObject::invokeMethod(controller, [controller, context, &result]() {
        result = controller->setup(context);
    }, Qt::BlockingQueuedConnection);
    return result;
}

void TaskLocalization::queueConfigureRuntimeController()
{
    auto *controller = m_runtimeController;
    if (!controller) {
        return;
    }
    const TaskLocalizeConfig cfg = m_config;
    if (controller->thread() == QThread::currentThread()) {
        controller->configure(cfg);
        return;
    }
    QMetaObject::invokeMethod(controller, [controller, cfg]() {
        controller->configure(cfg);
    }, Qt::QueuedConnection);
}

void TaskLocalization::queueSetActiveCameraNumber(int number)
{
    auto *controller = m_runtimeController;
    if (!controller) {
        return;
    }
    QMetaObject::invokeMethod(controller, [controller, number]() {
        controller->setActiveCameraNumber(number);
    }, Qt::QueuedConnection);
}

void TaskLocalization::queueSetActivePatternGroupNumber(int number)
{
    auto *controller = m_runtimeController;
    if (!controller) {
        return;
    }
    QMetaObject::invokeMethod(controller, [controller, number]() {
        controller->setActivePatternGroupNumber(number);
    }, Qt::QueuedConnection);
}

void TaskLocalization::queueHandlePlcValues(const QMap<QString, QVariant> &values)
{
    auto *controller = m_runtimeController;
    if (!controller) {
        return;
    }
    const auto copiedValues = values;
    QMetaObject::invokeMethod(controller, [controller, copiedValues]() {
        controller->handlePlcValues(copiedValues);
    }, Qt::QueuedConnection);
}

void TaskLocalization::wireMatchingWorkerSignals() {
    m_matchingWorker = new QObject();
    m_matchingWorker->moveToThread(matchingRunner);

    connect(matchingRunner, &QThread::finished, m_matchingWorker, &QObject::deleteLater);

    // commission matching handle, working on matching worker thread
    connect(this, &TaskLocalization::startCommissionMatchingRequest,
            m_matchingWorker, [this](std::shared_ptr<mtc::MatchGroup> group, cv::Mat image, CameraWorkspace workspace) {

        emit this->commissionMatchingFinished(m_pipeline.runMatchCommision(group, workspace, image));
    });
}

} // namespace vc::model
