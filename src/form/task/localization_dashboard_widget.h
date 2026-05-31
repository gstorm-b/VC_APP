#ifndef LOCALIZATION_DASHBOARD_WIDGET_H
#define LOCALIZATION_DASHBOARD_WIDGET_H

#include <QWidget>
#include "DockWidget.h"

#include "model/task_localization.h"
#include "form/task_widget.h"

namespace Ui {
class LocalizationDashboardWidget;
}

class LocalizationDashboardWidget : public ITaskWidget
{
    Q_OBJECT

public:
    explicit LocalizationDashboardWidget(std::shared_ptr<vc::model::ITask> task,
                                         ads::CDockWidget *dock = nullptr,
                                         QWidget *parent = nullptr);

    ~LocalizationDashboardWidget();

    void loadConfigToTask() override;
    void loadConfigToWidget() override;

private:
    void initWidget();
    void setupLamps();
    void pushSignalTagsFromConfig();
    void updateTaskContext();
    void updateTaskStateLabel();
    void setupResultTable();
    void updateCycleResult(const vc::model::LocalizationRuntimeController::CycleResult &result);
    void appendTaskLog(const vc::model::LocalizationRuntimeController::TaskLogEntry &entry);

    // Route a single live signal value to its dashboard visual (state lamps,
    // fault panel, or context value label). Monitor-list refresh is handled
    // separately at the call site.
    void applySignalToDashboard(const QString &name, const QVariant &value);
    // Drive the fault panel (frame_fault[active] + value labels + task lamp).
    void setFaultState(bool active, int faultCode);

private:
    Ui::LocalizationDashboardWidget *ui;

    vc::model::TaskLocalization *m_localizeTask{nullptr};
    vc::model::TaskLocalizeConfig m_config;

    int  m_lastFaultCode{0};
    bool m_taskFaultActive{false};
};

#endif // LOCALIZATION_DASHBOARD_WIDGET_H
