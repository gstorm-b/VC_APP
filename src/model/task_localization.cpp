#include "task_localization.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QRegularExpression>

#include "matching/match_group.h"
#include "matching/match_pattern.h"
#include "model/project.h"

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

    m_limitDeviceMap.insert(device::DeviceType::PLC, limit_comm_device);
    m_limitDeviceMap.insert(device::DeviceType::VisionOutput, limit_vision_output_device);
    m_limitDeviceMap.insert(device::DeviceType::Camera, limit_num_camera);

    m_patternManager = new mtc::PatternGroupManager(this);
    m_runtimeController = new LocalizationRuntimeController(this);
    m_runtimeController->setPipeline(&m_pipeline);
    m_runtimeController->configure(m_config);

    connect(m_runtimeController, &LocalizationRuntimeController::signalChanged,
            this, &ITask::signalChanged);
    connect(m_runtimeController, &LocalizationRuntimeController::cameraNumberChanged,
            this, &TaskLocalization::onSignalChangeCameraNumber);
    connect(m_runtimeController, &LocalizationRuntimeController::patternNumberChanged,
            this, &TaskLocalization::onSignalChangePatternNumber);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeRecovering,
            this, &TaskLocalization::onRuntimeRecovering);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeReady,
            this, &TaskLocalization::onRuntimeReady);
    connect(m_runtimeController, &LocalizationRuntimeController::runtimeFault,
            this, &TaskLocalization::onRuntimeFault);

    matchingRunner = new QThread();
    wireMatchingCommissionSignals();
    matchingRunner->start();
}

void TaskLocalization::setTaskLocalizeConfig(const TaskLocalizeConfig &cfg)
{
    m_config = cfg;
    this->setTaskConfig(&m_config);
    if (m_runtimeController)
        m_runtimeController->configure(m_config);
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
    setupTask();
    if (!m_runtimeController || !m_runtimeController->isValid()) {
        transitionTaskState(TaskState::Faulted,
                            QStringLiteral("Runtime start aborted: setupTask failed"));
        return;
    }

    ITask::beginRuntime(mergeToTaskThread);
}

// ── Typed runner helpers ──────────────────────────────────────────────────────

vc::runtime::CameraRunner *TaskLocalization::cameraRunner(const QString &deviceId) const
{
    return qobject_cast<vc::runtime::CameraRunner *>(
        taskRunner()->runnerFor(deviceId));
}

vc::runtime::PlcRunner *TaskLocalization::plcRunner(const QString &deviceId) const
{
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
    if (!m_patternManager) return map;

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
    return map;
}

bool TaskLocalization::loadTaskImageMap(QMap<QString, cv::Mat> &mapping) {
    if (!m_patternManager) return false;
    if (mapping.isEmpty())  return true;   // nothing to inject

    bool allOk = true;
    for (auto it = mapping.cbegin(); it != mapping.cend(); ++it) {
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

    const auto result = m_runtimeController->setup(this);
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

    if (!transitionTaskState(TaskState::RunningCycle,
                             QStringLiteral("Localization cycle started"))) {
        return;
    }

    m_runtimeController->execute();

    if (taskState() == TaskState::RunningCycle) {
        transitionTaskState(TaskState::Ready,
                            QStringLiteral("Localization cycle completed"));
    }
}

void TaskLocalization::setCameraNumber(int number) {
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
    if (device->deviceType() != device::DeviceType::Camera) {
        LOG_USER_WARN << tr("Cannot change camera, device %1 with id %2 isn't camera type")
                             .arg(number).arg(cam_id);
        return;
    }

    device::CameraDevice* camera = qobject_cast<device::CameraDevice*>(device.get());
    if (!camera) {
        LOG_USER_WARN << tr("Cannot change camera, device id %1 is null").arg(cam_id);
        return;
    }

    // select camera
    if (m_selectedCamera) {
        if (m_selectedCamera->isDeviceConnected()) {
            m_selectedCamera->deviceDisconnect();

            // wait until disconnect current camera before connect another camera
            connect(m_selectedCamera.get(), &device::IDevice::connectStatusChanged,
                    this, &TaskLocalization::waitReconnectCameraHandle, Qt::SingleShotConnection);

            m_nextConnectCamera.reset(camera);
            if (m_runtimeController) {
                m_runtimeController->setActiveCameraNumber(number);
            }
            return;
        }
    }

    m_nextConnectCamera.reset(camera);
    m_selectedCamera.reset(camera);

    if (m_runtimeController) {
        m_runtimeController->setActiveCameraNumber(number);
    }
}

void TaskLocalization::setPatternNumber(int number) {

}

void TaskLocalization::onCommDeviceValueChanged(QMap<QString, QVariant> values) {
    if (m_runtimeController)
        m_runtimeController->handlePlcValues(values);
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


void TaskLocalization::waitReconnectCameraHandle(device::ConnectStatus status) {
    if ((status == device::ConnectStatus::Disconnected) || (status == device::ConnectStatus::LostConnected)) {

        m_selectedCamera = m_nextConnectCamera;
        m_selectedCamera->deviceConnect();
        return;
    }

    connect(m_selectedCamera.get(), &device::IDevice::connectStatusChanged,
            this, &TaskLocalization::waitReconnectCameraHandle, Qt::SingleShotConnection);
}

void TaskLocalization::selectedCameraConnectStatusChanged(device::ConnectStatus status) {

    switch (status) {
    case device::ConnectStatus::LostConnected:
        // handle lost connect

        break;
    case device::ConnectStatus::ConnectFailed:
        // handle connect failed

        break;
    case device::ConnectStatus::Connected:


        break;
    case device::ConnectStatus::Disconnected:

        break;
    default:
        break;
    }

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


void TaskLocalization::wireMatchingCommissionSignals() {
    QObject *worker = new QObject();
    worker->moveToThread(matchingRunner);

    // delete worker and matching commision thread
    connect(this, &TaskLocalization::destroyed, worker, [worker]() {
        QThread* worker_thread = worker->thread();
        if (worker_thread->isRunning()) {
            worker_thread->quit();
        }
        worker->deleteLater();
        worker_thread->deleteLater();
    });

    // commission matching handle, working on matching worker thread
    connect(this, &TaskLocalization::startCommissionMatchingRequest,
            worker, [this](std::shared_ptr<mtc::MatchGroup> group, cv::Mat image) {

        emit this->commissionMatchingFinished(m_pipeline.runMatch(group, image));
    });
}

} // namespace vc::model
