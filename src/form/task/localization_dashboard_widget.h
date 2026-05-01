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
    Ui::LocalizationDashboardWidget *ui;
    ads::CDockWidget *m_dock{nullptr};
    std::shared_ptr<vc::model::ITask> m_task;
    vc::model::TaskLocalization *m_localizeTask;
    vc::model::TaskLocalizeConfig m_config;

};

#endif // LOCALIZATION_DASHBOARD_WIDGET_H
