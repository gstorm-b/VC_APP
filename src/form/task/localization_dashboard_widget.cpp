#include "localization_dashboard_widget.h"
#include "ui_localization_dashboard_widget.h"

#include "widgets/signals_monitor_widget.h"

#include "logger/app_logger.h"

#include <QMetaProperty>

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
    { "nActiveCamera",     "Active camera",     SignalsMonitorWidget::Type::Number },
    { "nActivePattern",    "Active pattern",    SignalsMonitorWidget::Type::Number },
    { "nDetectedNumber",   "Detected count",    SignalsMonitorWidget::Type::Number },
    // Bool signals
    { "bCameraValid",      "Camera valid",      SignalsMonitorWidget::Type::Bool   },
    { "bPatternValid",     "Pattern valid",     SignalsMonitorWidget::Type::Bool   },
    { "bTaskReady",        "Task ready",        SignalsMonitorWidget::Type::Bool   },
    { "bExecuteTrigger",   "Execute trigger",   SignalsMonitorWidget::Type::Bool   },
    { "bMatchingFinished", "Matching finished", SignalsMonitorWidget::Type::Bool   },
    { "bMatchingBusy",     "Matching busy",     SignalsMonitorWidget::Type::Bool   },
    { "bMatchingDetected", "Matching detected", SignalsMonitorWidget::Type::Bool   },
    { "bMatchingLowArea",  "Low binary area",   SignalsMonitorWidget::Type::Bool   },
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

    // ── Signal monitor: append fixed schema ────────────────────────────
    for (const auto &spec : kSignalRows) {
        ui->listWidget_tag_status->appendRow(
            QString::fromUtf8(spec.internalName),
            QString::fromUtf8(spec.displayName),
            spec.type);
    }

    // ── User-initiated writes ──────────────────────────────────────────
    // TODO(follow-up): route to TaskLocalization's signal-write API once
    // it lands. For now we just log so the path is visible in user logs.
    connect(ui->listWidget_tag_status,
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
    });

    // ── Live signal value updates ──────────────────────────────────────
    // The dashboard does NOT subscribe to the communication device directly;
    // TaskLocalization exposes a unified signalChanged(name, value) that the
    // monitor refreshes from. The widget refreshes by signal name (the
    // internalName), not by PLC tag, so we just dispatch by configured type.
    //
    // TODO(follow-up): TaskLocalization::signalChanged and its emit-path
    // (observing the PLC and forwarding) are tracked in a separate request.
    // Until that lands this connect references an undeclared signal — the
    // wiring is intentional and the build will green up once the signal is
    // added to TaskLocalization.
    connect(m_localizeTask, &TaskLocalization::signalChanged,
            this, [this](const QString &name, const QVariant &value) {
        switch (typeOf(name)) {
        case SignalsMonitorWidget::Type::Bool:
            ui->listWidget_tag_status->refreshBool(name, value.toBool());
            break;
        case SignalsMonitorWidget::Type::Number:
            ui->listWidget_tag_status->refreshNumber(name, value.toInt());
            break;
        }
    });

    // TODO(follow-up): wire device-connection status through TaskLocalization
    // too (TaskLocalization::commDeviceConnectStatusChanged(bool) or similar).
    // Until then the Modify button stays disabled so users cannot trigger
    // a write that has nowhere to land.
    ui->listWidget_tag_status->setDeviceConnected(false);

    loadConfigToWidget();
}

void LocalizationDashboardWidget::loadConfigToWidget() {
    if (!m_localizeTask) return;
    m_config = m_localizeTask->taskLocalizeConfig();
    pushSignalTagsFromConfig();
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
        ui->listWidget_tag_status->setRowTag(name, readConfigField(m_config, name));
    }
}
