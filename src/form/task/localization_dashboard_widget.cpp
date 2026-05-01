#include "localization_dashboard_widget.h"
#include "ui_localization_dashboard_widget.h"

LocalizationDashboardWidget::LocalizationDashboardWidget(std::shared_ptr<vc::model::ITask> task,
                                                         ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
    ui(new Ui::LocalizationDashboardWidget) {

    ui->setupUi(this);
}

LocalizationDashboardWidget::~LocalizationDashboardWidget()
{
    delete ui;
}

void LocalizationDashboardWidget::loadConfigToTask() {

}

void LocalizationDashboardWidget::loadConfigToWidget() {

}
