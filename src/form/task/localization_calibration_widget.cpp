#include "localization_calibration_widget.h"
#include "ui_localization_calibration_widget.h"

LocalizationCalibrationWidget::LocalizationCalibrationWidget(std::shared_ptr<vc::model::ITask> task,
                                                             ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
    ui(new Ui::LocalizationCalibrationWidget) {

    ui->setupUi(this);
}

LocalizationCalibrationWidget::~LocalizationCalibrationWidget()
{
    delete ui;
}

void LocalizationCalibrationWidget::loadConfigToTask() {

}

void LocalizationCalibrationWidget::loadConfigToWidget() {

}
