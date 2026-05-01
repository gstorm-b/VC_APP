#ifndef LOCALIZATION_TASK_WIDGET_H
#define LOCALIZATION_TASK_WIDGET_H

#include <QWidget>

#include "model/task_localization.h"
#include "form/task_widget.h"

namespace Ui {
class LocalizationTaskWidget;
}

class LocalizationTaskWidget : public ITaskWidget {
    Q_OBJECT

public:
    explicit LocalizationTaskWidget(std::shared_ptr<vc::model::ITask> task,
                                    ads::CDockWidget *dock = nullptr,
                                    QWidget *parent = nullptr);
    ~LocalizationTaskWidget();

    void loadConfigToTask() override;
    void loadConfigToWidget() override;

private:
    void initWidget();
    void updateCommDeviceInfo();
    void updateOutputDeviceInfo();
    void populateBrowser();
    void populateBrowser_Task();
    void populateBrowser_Config();
    void updateCameraMappingToWidget();

private slots:
    void onSelectCommDeviceClicked();
    void onUpdateCompleter();
    void onSelectOutputDeviceClicked();

    void onCameraMappingChanged(const QMap<int, QString> &mapping);

    void onPropertyManagerValueChanged(QtProperty *property, const QVariant &variant);
    void saveConfig();

private:
    Ui::LocalizationTaskWidget *ui;
    vc::model::TaskLocalization *m_localizeTask;
    vc::model::TaskLocalizeConfig m_config;

    bool m_populating_browser{false};

    QStringList m_BitsAddressList;
    QStringList m_WordsAddressList;

    std::shared_ptr<vc::device::IDevice> m_commDevice;
};

#endif // LOCALIZATION_TASK_WIDGET_H
