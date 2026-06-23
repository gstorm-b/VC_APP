#include "localization_dashboard_widget.h"
#include "ui_localization_dashboard_widget.h"

#include "widgets/signals_monitor_widget.h"
#include "widgets/task_event_log_widget.h"
#include "widgets/image_widget/image_view_only.h"
#include "form/widgets/status_lamp.h"

#include "logger/app_logger.h"
#include "model/project.h"
#include "model/localization_fault_code.h"
#include "device/device_manager.h"
#include "runtime/idevice_runner.h"
#include "runtime/task_runner.h"

#include <QColor>
#include <QMetaProperty>
#include <QStyle>
#include <QTableWidgetItem>

#include <utility>

using vc::model::TaskLocalization;
using vc::model::TaskLocalizeConfig;

namespace {

// Schema for the signal monitor — internalName, displayName and type
// declared inline. Display names live here (per yêu cầu) rather than being
// resolved through Q_CLASSINFO so a missing/renamed field is a visible diff
// in code review.
struct SignalRowSpec {
    const char *internalName;
    const char *displayName;
    SignalsMonitorWidget::Type type;
};

constexpr SignalRowSpec kSignalRows[] = {
    // Number signals
    { "nActiveCamera",       "Active camera",       SignalsMonitorWidget::Type::Number },
    { "nActivePatternGroup", "Active pattern group", SignalsMonitorWidget::Type::Number },
    { "nDetectedNumber",     "Detected count",      SignalsMonitorWidget::Type::Number },
    { "nFaultCode",          "Fault code",          SignalsMonitorWidget::Type::Number },
    // Bool signals
    { "bCameraValid",      "Camera valid",      SignalsMonitorWidget::Type::Bool   },
    { "bPatternValid",     "Pattern valid",     SignalsMonitorWidget::Type::Bool   },
    { "bTaskReady",        "Task ready",        SignalsMonitorWidget::Type::Bool   },
    { "bExecuteTrigger",   "Execute trigger",   SignalsMonitorWidget::Type::Bool   },
    { "bMatchingFinished", "Matching finished", SignalsMonitorWidget::Type::Bool   },
    { "bMatchingBusy",     "Matching busy",     SignalsMonitorWidget::Type::Bool   },
    { "bMatchingDetected", "Matching detected", SignalsMonitorWidget::Type::Bool   },
    { "bMatchingLowArea",  "Low binary area",   SignalsMonitorWidget::Type::Bool   },
    { "bTaskFault",        "Task fault",        SignalsMonitorWidget::Type::Bool   },
};

QString readConfigField(const TaskLocalizeConfig &cfg, const QString &internalName) {
    const QMetaObject &meta = TaskLocalizeConfig::staticMetaObject;
    int idx = meta.indexOfProperty(internalName.toUtf8().constData());
    if (idx < 0) return {};
    return meta.property(idx).readOnGadget(&cfg).toString();
}

SignalsMonitorWidget::Type typeOf(const QString &internalName) {
    for (const auto &s : kSignalRows) {
        if (internalName == QLatin1String(s.internalName)) return s.type;
    }
    return SignalsMonitorWidget::Type::Number;   // fallback; unknown name is logged by widget
}

// Map a runtime log severity string to the operator-log severity level.
TaskEventLevel severityToLevel(const QString &severity) {
    const QString s = severity.trimmed().toUpper();
    if (s == QLatin1String("ERROR") || s == QLatin1String("FATAL") ||
        s == QLatin1String("CRITICAL"))
        return TaskEventLevel::Error;
    if (s == QLatin1String("WARN") || s == QLatin1String("WARNING"))
        return TaskEventLevel::Warning;
    if (s == QLatin1String("OK") || s == QLatin1String("SUCCESS") ||
        s == QLatin1String("DONE"))
        return TaskEventLevel::Success;
    return TaskEventLevel::Info;
}

QString faultCodeText(int code) {
    const auto name = vc::model::localizationFaultCodeName(
        static_cast<vc::model::LocalizationFaultCode>(code));
    return QStringLiteral("%1 — %2").arg(code).arg(name);
}

} // namespace

LocalizationDashboardWidget::LocalizationDashboardWidget(std::shared_ptr<vc::model::ITask> task,
                                                         ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
    ui(new Ui::LocalizationDashboardWidget) {

    ui->setupUi(this);
    initWidget();
}

LocalizationDashboardWidget::~LocalizationDashboardWidget()
{
    delete ui;
}

void LocalizationDashboardWidget::initWidget() {
    m_localizeTask = static_cast<TaskLocalization *>(m_task.get());
    if (!m_localizeTask) {
        LOG_DEV_ERR << "LocalizationDashboardWidget: task is not TaskLocalization";
        return;
    }

    m_config = m_localizeTask->taskLocalizeConfig();

    setupLamps();
    setupConnectionLamps();
    setupResultTable();
    setFaultState(false, 0);
    updateTaskContext();
    updateTaskStateLabel();

    // ── Signal monitor: append fixed schema ────────────────────────────
    for (const auto &spec : kSignalRows) {
        ui->wg_signal_monitor->appendRow(
            QString::fromUtf8(spec.internalName),
            QString::fromUtf8(spec.displayName),
            spec.type);
    }

    // ── User-initiated writes ──────────────────────────────────────────
    // Dashboard v1 is read-only. Keep the signal visible in logs in case a
    // row becomes editable by mistake, but do not route writes from here.
    connect(ui->wg_signal_monitor,
            &SignalsMonitorWidget::requestWriteValue,
            this, [](const QString &name, const QVariant &v) {
        LOG_USER_WARN << "SignalsMonitor write requested (not wired):"
                      << name << "=" << v.toString();
    });

    // ── React to assignment changes from the task ──────────────────────
    connect(m_localizeTask, &vc::model::ITask::devicesChanged,
            this, [this] {
        m_config = m_localizeTask->taskLocalizeConfig();
        pushSignalTagsFromConfig();
        updateTaskContext();
        rebuildConnectionWiring();
    });
    connect(m_localizeTask, &vc::model::ITask::taskStateChanged,
            this, [this] {
        updateTaskStateLabel();
    });
    connect(m_localizeTask, &TaskLocalization::cycleResultUpdated,
            this, &LocalizationDashboardWidget::updateCycleResult);
    connect(m_localizeTask, &TaskLocalization::taskLogAppended,
            this, &LocalizationDashboardWidget::appendTaskLog);

    // ── Live signal value updates ──────────────────────────────────────
    // The dashboard does NOT subscribe to the communication device directly;
    // TaskLocalization exposes a unified signalChanged(name, value) that the
    // monitor refreshes from. The widget refreshes by signal name (the
    // internalName), not by PLC tag, so we just dispatch by configured type.
    //
    connect(m_localizeTask, &TaskLocalization::signalChanged,
            this, [this](const QString &name, const QVariant &value) {
        switch (typeOf(name)) {
        case SignalsMonitorWidget::Type::Bool:
            ui->wg_signal_monitor->refreshBool(name, value.toBool());
            break;
        case SignalsMonitorWidget::Type::Number:
            ui->wg_signal_monitor->refreshNumber(name, value.toInt());
            break;
        }
        applySignalToDashboard(name, value);
    });

    // Dashboard v1 remains read-only, so writes stay disabled even when the
    // runtime device is connected.
    ui->wg_signal_monitor->setDeviceConnected(false);

    loadConfigToWidget();
}

void LocalizationDashboardWidget::setupLamps() {
    ui->lamp_task->setLampName(tr("Task"));
    ui->lamp_camera->setLampName(tr("Camera"));
    ui->lamp_pattern->setLampName(tr("Pattern"));
    ui->lamp_cycle->setLampName(tr("Cycle"));

    ui->lamp_task->setStatus(StatusLamp::Status::Off, tr("—"));
    ui->lamp_camera->setStatus(StatusLamp::Status::Off, tr("—"));
    ui->lamp_pattern->setStatus(StatusLamp::Status::Off, tr("—"));
    ui->lamp_cycle->setStatus(StatusLamp::Status::Off, tr("Idle"));
}

void LocalizationDashboardWidget::setupConnectionLamps() {
    ui->lamp_connection_plc->setLampName(tr("PLC"));
    ui->lamp_connection_camera->setLampName(tr("Camera"));
    ui->lamp_connection_output->setLampName(tr("Output"));

    ui->lamp_connection_plc->setStatus(StatusLamp::Status::Off, tr("—"));
    ui->lamp_connection_camera->setStatus(StatusLamp::Status::Off, tr("—"));
    ui->lamp_connection_output->setStatus(StatusLamp::Status::Off, tr("—"));

    rebuildConnectionWiring();
}

void LocalizationDashboardWidget::rebuildConnectionWiring() {
    // Drop any previous runner subscriptions before re-resolving devices.
    for (const auto &conn : std::as_const(m_connectionConns))
        QObject::disconnect(conn);
    m_connectionConns.clear();

    if (!m_localizeTask) return;

    const auto &bindings = m_config.d->m_deviceBindings;
    wireConnectionLamp(ui->lamp_connection_plc,    bindings.primaryPlcDeviceId());
    wireConnectionLamp(ui->lamp_connection_camera, resolveActiveCameraDeviceId());
    wireConnectionLamp(ui->lamp_connection_output, bindings.visionOutputDeviceId());
}

void LocalizationDashboardWidget::wireConnectionLamp(StatusLamp *lamp,
                                                     const QString &deviceId) {
    if (!lamp) return;

    if (deviceId.isEmpty()) {
        lamp->setStatus(StatusLamp::Status::Off, tr("Not set"));
        return;
    }

    auto *taskRunner = m_localizeTask->taskRunner();
    auto *runner = taskRunner ? taskRunner->runnerFor(deviceId) : nullptr;
    if (!runner) {
        // Bound, but no runner yet (device not registered / wrong phase).
        lamp->setStatus(StatusLamp::Status::Off, tr("—"));
        return;
    }

    // Seed the lamp from the current status. connectStatus() is a plain enum
    // read; a momentarily stale value is acceptable for an indicator and is
    // corrected by the next forwarded change below.
    applyConnectStatusToLamp(
        lamp, runner->device() ? runner->device()->connectStatus()
                               : vc::device::ConnectStatus::NoConnection);

    // The runner forwards the device's connectStatusChanged onto the GUI thread
    // (QueuedConnection), so updating the lamp from this slot is thread-safe.
    m_connectionConns.append(connect(
        runner, &vc::runtime::IDeviceRunner::connectStatusChanged, this,
        [this, lamp](vc::device::ConnectStatus status) {
            applyConnectStatusToLamp(lamp, status);
        }));
}

void LocalizationDashboardWidget::applyConnectStatusToLamp(
    StatusLamp *lamp, vc::device::ConnectStatus status) {
    if (!lamp) return;
    using CS = vc::device::ConnectStatus;
    switch (status) {
    case CS::Connected:
        lamp->setStatus(StatusLamp::Status::Ok, tr("Connected"));
        break;
    case CS::Connecting:
        lamp->setStatus(StatusLamp::Status::Warning, tr("Connecting"));
        break;
    case CS::LostConnected:
        lamp->setStatus(StatusLamp::Status::Error, tr("Lost"));
        break;
    case CS::ConnectFailed:
        lamp->setStatus(StatusLamp::Status::Error, tr("Failed"));
        break;
    case CS::Disconnected:
        lamp->setStatus(StatusLamp::Status::Off, tr("Disconnected"));
        break;
    case CS::NoConnection:
    default:
        lamp->setStatus(StatusLamp::Status::Off, tr("—"));
        break;
    }
}

QString LocalizationDashboardWidget::resolveActiveCameraDeviceId() const {
    const QMap<int, QString> cameras = m_config.d->m_deviceBindings.cameraNumberMap();
    if (cameras.isEmpty()) return QString();
    // Prefer the live active camera when it maps to a bound device; otherwise
    // fall back to the first bound camera (QMap iterates by ascending number).
    if (cameras.contains(m_activeCameraNumber))
        return cameras.value(m_activeCameraNumber);
    return cameras.first();
}

void LocalizationDashboardWidget::loadConfigToWidget() {
    if (!m_localizeTask) return;
    m_config = m_localizeTask->taskLocalizeConfig();
    pushSignalTagsFromConfig();
    updateTaskContext();
    updateTaskStateLabel();
}

void LocalizationDashboardWidget::loadConfigToTask() {
    // Dashboard is read-only — no config to push back to the task.
}

void LocalizationDashboardWidget::pushSignalTagsFromConfig() {
    // Tag string is still sourced from the config — the widget uses it only
    // to decide whether to render "(not mapped)". Live values arrive via
    // TaskLocalization::signalChanged keyed by signal name, not by tag.
    for (const auto &spec : kSignalRows) {
        const QString name = QString::fromUtf8(spec.internalName);
        ui->wg_signal_monitor->setRowTag(name, readConfigField(m_config, name));
    }
}

void LocalizationDashboardWidget::updateTaskContext()
{
    if (!m_localizeTask) return;

    auto deviceName = [this](const QString &deviceId) {
        if (deviceId.isEmpty()) return QStringLiteral("—");
        auto project = m_localizeTask->project();
        auto manager = project ? project->deviceManager() : nullptr;
        auto device = manager ? manager->deviceById(deviceId) : nullptr;
        return device ? QStringLiteral("%1 (%2)").arg(device->name(), deviceId)
                      : QStringLiteral("%1 (missing)").arg(deviceId);
    };

    const auto &bindings = m_config.d->m_deviceBindings;
    // Captions are static in the .ui; the widget only writes value labels.
    ui->lbl_val_vision_device->setText(deviceName(bindings.visionOutputDeviceId()));
    ui->lbl_val_plc_device->setText(deviceName(bindings.primaryPlcDeviceId()));
    ui->lbl_val_camera->setText(QStringLiteral("—"));
    ui->lbl_val_pattern_group->setText(QStringLiteral("—"));
}

void LocalizationDashboardWidget::updateTaskStateLabel()
{
    if (!m_localizeTask) return;
    const auto state = m_localizeTask->taskState();

    // The "Cycle" lamp reflects runtime cycle activity derived from task state.
    StatusLamp::Status status;
    switch (state) {
    case vc::model::TaskState::Faulted:
        status = StatusLamp::Status::Error;
        break;
    case vc::model::TaskState::RunningCycle:
    case vc::model::TaskState::Recovering:
        status = StatusLamp::Status::Warning;
        break;
    case vc::model::TaskState::Ready:
        status = StatusLamp::Status::Ok;
        break;
    default:
        status = StatusLamp::Status::Off;
        break;
    }
    ui->lamp_cycle->setStatus(status, vc::model::taskStateDisplayName(state));
}

void LocalizationDashboardWidget::applySignalToDashboard(const QString &name,
                                                         const QVariant &value)
{
    if (name == QLatin1String("bCameraValid")) {
        const bool ok = value.toBool();
        ui->lamp_camera->setStatus(ok ? StatusLamp::Status::Ok : StatusLamp::Status::Off,
                                   ok ? tr("Valid") : tr("Invalid"));
    } else if (name == QLatin1String("bPatternValid")) {
        const bool ok = value.toBool();
        ui->lamp_pattern->setStatus(ok ? StatusLamp::Status::Ok : StatusLamp::Status::Off,
                                    ok ? tr("Valid") : tr("Invalid"));
    } else if (name == QLatin1String("bTaskReady")) {
        // Don't override a fault-driven Error state on the task lamp.
        if (!m_taskFaultActive) {
            const bool ready = value.toBool();
            ui->lamp_task->setStatus(ready ? StatusLamp::Status::Ok : StatusLamp::Status::Off,
                                     ready ? tr("Ready") : tr("Not ready"));
        }
    } else if (name == QLatin1String("bMatchingBusy")) {
        const bool busy = value.toBool();
        ui->lamp_cycle->setStatus(busy ? StatusLamp::Status::Warning : StatusLamp::Status::Off,
                                  busy ? tr("Running") : tr("Idle"));
    } else if (name == QLatin1String("bTaskFault")) {
        setFaultState(value.toBool(), m_lastFaultCode);
    } else if (name == QLatin1String("nFaultCode")) {
        m_lastFaultCode = value.toInt();
        ui->lbl_val_fault_code->setText(faultCodeText(m_lastFaultCode));
    } else if (name == QLatin1String("nActiveCamera")) {
        const int number = value.toInt();
        ui->lbl_val_camera->setText(QString::number(number));
        if (number != m_activeCameraNumber) {
            m_activeCameraNumber = number;
            // The camera lamp follows the active camera; re-resolve its device.
            rebuildConnectionWiring();
        }
    } else if (name == QLatin1String("nActivePatternGroup")) {
        ui->lbl_val_pattern_group->setText(QString::number(value.toInt()));
    }
}

void LocalizationDashboardWidget::setFaultState(bool active, int faultCode)
{
    m_taskFaultActive = active;
    m_lastFaultCode   = faultCode;

    ui->lbl_val_fault_flag->setText(active ? QStringLiteral("true")
                                           : QStringLiteral("false"));
    ui->lbl_val_fault_code->setText(faultCodeText(faultCode));

    // Drive the QFrame#frame_fault[active] attribute selector (Rule 4.5).
    ui->frame_fault->setProperty("active", active);
    ui->frame_fault->style()->unpolish(ui->frame_fault);
    ui->frame_fault->style()->polish(ui->frame_fault);
    ui->frame_fault->update();

    // The task lamp surfaces the fault as an Error; cleared faults fall back
    // to whatever the next bTaskReady update reports.
    if (active)
        ui->lamp_task->setStatus(StatusLamp::Status::Error, tr("Fault"));
}

void LocalizationDashboardWidget::setupResultTable()
{
    ui->tbl_result->setColumnCount(11);
    ui->tbl_result->setHorizontalHeaderLabels({
        tr("#"),
        tr("Pattern"),
        tr("Score"),
        tr("Img X"),
        tr("Img Y"),
        tr("Img R"),
        tr("World X"),
        tr("World Y"),
        tr("World Z"),
        tr("World R"),
        tr("Status"),
    });
    ui->tbl_result->setRowCount(0);
}

void LocalizationDashboardWidget::updateCycleResult(
    const vc::model::LocalizationRuntimeController::CycleResult &result)
{
    ui->lbl_kpi_detected_val->setText(QString::number(result.detectedNumber));
    ui->lbl_kpi_sent_val->setText(QString::number(result.sentNumber));
    ui->lbl_kpi_cycle_time_val->setText(QStringLiteral("%1 ms").arg(result.matchingTimeMs, 0, 'f', 1));
    ui->lbl_kpi_low_area_val->setText(result.lowArea ? tr("Yes") : tr("No"));

    // ImageViewOnly owns its own scene + pixmap item and handles cv::Mat →
    // QPixmap conversion and fit-to-view internally (same path the patterns
    // widget uses for imageView_Raw / imageView_Result).
    ui->gv_match_view->removeAllROI();
    if (result.displayImage.empty()) {
        ui->gv_match_view->clearCurrentImage();
    } else {
        cv::Mat display = result.displayImage;   // loadImageOpenCv takes a non-const ref
        ui->gv_match_view->loadImageOpenCv(display, true);
        drawConditionRoiOverlay(display.cols, display.rows);
    }

    ui->tbl_result->setRowCount(result.rows.size());
    for (int row = 0; row < result.rows.size(); ++row) {
        const auto &r = result.rows.at(row);
        const QStringList values = {
            QString::number(r.index),
            r.patternName,
            QString::number(r.score, 'f', 3),
            QString::number(r.imageX, 'f', 2),
            QString::number(r.imageY, 'f', 2),
            QString::number(r.imageR, 'f', 2),
            QString::number(r.world.x, 'f', 3),
            QString::number(r.world.y, 'f', 3),
            QString::number(r.world.z, 'f', 3),
            QString::number(r.world.r, 'f', 3),
            r.status,
        };
        for (int col = 0; col < values.size(); ++col) {
            ui->tbl_result->setItem(row, col, new QTableWidgetItem(values.at(col)));
        }
    }
}

void LocalizationDashboardWidget::drawConditionRoiOverlay(int imgW, int imgH)
{
    if (!m_localizeTask || imgW <= 0 || imgH <= 0) return;

    const QString camId = resolveActiveCameraDeviceId();
    if (camId.isEmpty()) return;

    // Read the workspace fresh from the task: workspace edits made after this
    // widget was constructed are not mirrored into the cached m_config.
    const vc::model::CameraWorkspace ws =
        m_localizeTask->taskLocalizeConfig().cameraWorkspace(camId);
    if (!ws.useConditionWorkspace || !ws.hasConditionRoi()) return;

    // The result image is cropped to the working ROI when that workspace is
    // active, so shift the condition ROI (full-frame coords) into the crop.
    float ox = 0.0f, oy = 0.0f;
    if (ws.useWorkspace && ws.hasRoi()) {
        ox = ws.roi.x;
        oy = ws.roi.y;
    }

    const int tlx = qMax(0, qRound(ws.conditionRoi.x - ox));
    const int tly = qMax(0, qRound(ws.conditionRoi.y - oy));
    const int brx = qMin(imgW, qRound(ws.conditionRoi.x - ox + ws.conditionRoi.width));
    const int bry = qMin(imgH, qRound(ws.conditionRoi.y - oy + ws.conditionRoi.height));
    if (brx <= tlx || bry <= tly) return;

    // Info blue — matches the patterns raw-view condition overlay.
    ui->gv_match_view->addROI(tlx, tly, brx, bry, QColor(0x40, 0xA8, 0xE0));
}

void LocalizationDashboardWidget::appendTaskLog(
    const vc::model::LocalizationRuntimeController::TaskLogEntry &entry)
{
    TaskEvent ev;
    ev.timestamp = entry.timestamp;
    ev.level     = severityToLevel(entry.severity);
    ev.message   = entry.message;
    // The operator log is task-scoped; no per-component source tag for now.
    ui->log_task_view->appendEvent(ev);
}
