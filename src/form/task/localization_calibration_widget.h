#ifndef LOCALIZATION_CALIBRATION_WIDGET_H
#define LOCALIZATION_CALIBRATION_WIDGET_H

#include <QWidget>
#include "DockWidget.h"

#include "model/task_localization.h"
#include "form/task_widget.h"

namespace Ui {
class LocalizationCalibrationWidget;
}

class LocalizationCalibrationWidget : public ITaskWidget
{
    Q_OBJECT

public:
    explicit LocalizationCalibrationWidget(std::shared_ptr<vc::model::ITask> task,
                                           ads::CDockWidget *dock = nullptr,
                                           QWidget *parent = nullptr);
    ~LocalizationCalibrationWidget();

    void loadConfigToTask() override;
    void loadConfigToWidget() override;

private:
    Ui::LocalizationCalibrationWidget *ui;    
    ads::CDockWidget *m_dock{nullptr};
    std::shared_ptr<vc::model::ITask> m_task;
    vc::model::TaskLocalization *m_localizeTask;
    vc::model::TaskLocalizeConfig m_config;
};

#endif // LOCALIZATION_CALIBRATION_WIDGET_H
