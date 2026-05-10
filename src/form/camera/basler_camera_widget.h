#ifndef BASLER_CAMERA_WIDGET_H
#define BASLER_CAMERA_WIDGET_H

#include <QWidget>
#include "DockWidget.h"
#include "form/camera/basler_cam_select_dialog.h"

#include "widgets/qtpropertybrowser/qtpropertymanager.h"
#include "widgets/qtpropertybrowser/qtvariantproperty.h"
#include "widgets/qtpropertybrowser/qttreepropertybrowser.h"

#include "device/idevice.h"
#include "device/device_manager.h"
#include "device/camera/camera_basler_gige.h"

#include "runtime/camera_runner.h"
#include "form/device_widget.h"

namespace Ui {
class BaslerCameraWidget;
}

class BaslerCameraWidget : public IDeviceWidget
{
    Q_OBJECT

public:
    // Runner is owned by TaskRunner (not by this widget).  It must outlive
    // this widget — TaskRunner guarantees this as long as the device stays
    // assigned to the task.  Pass nullptr only if the caller explicitly
    // disables thread-routed access (rare; status lamp will stay disabled).
    explicit BaslerCameraWidget(std::shared_ptr<vc::device::IDevice> dv,
                                vc::runtime::CameraRunner *runner,
                                ads::CDockWidget *dock = nullptr,
                                QWidget *parent = nullptr);
    ~BaslerCameraWidget();

    QString deviceId() override;
    void loadConfigToDevice() override;
    void loadConfigToWidget() override;
    void setDockWidget(ads::CDockWidget *dock);

private:
    void initCameraWiget();
    void populateBrowser_Device(vc::device::IDevice *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser);
    void populateBrowser_BaslerConfig(vc::device::BaslerGigeCfg *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser);
    void setDoublePropertyLimit(QtVariantProperty *variant, double &max, double &min);
    void setIntPropertyLimit(QtVariantProperty *variant, int &max, int &min);
    void updateDockTitle();

private slots:
    void onPropertyValueChanged(QtProperty *property, const QVariant &variant);
    void onCameraConnectStatusChanged(vc::device::ConnectStatus status);

    void cameraSelectionFinished(bool isAccept, Pylon::CDeviceInfo device);
    void btn_choose_camera_clicked();
    void btn_connect_clicked();
    void btn_trigger_clicked();
    // void btn_auto_shot_clicked();
    // void btn_backlight_toggle_clicked();
    // void btn_save_image_clicked();

private:
    void onCameraConnected();
    void onCameraDisconnected();
    void onCameraGrabFinished(vc::device::GrabResult result);
    void populateBrowser();

private:
    Ui::BaslerCameraWidget *ui;
    std::shared_ptr<vc::device::IDevice> m_device;
    vc::device::BaslerGigECamera *m_camera;
    ads::CDockWidget *m_dock{nullptr};

    vc::device::BaslerGigeCfg m_params;
    QList<vc::device::BaslerGigeCfg> m_cam_list;

    BaslerCamSelectDialog *m_camera_select_dialog{nullptr};

    // Not owned — provided by TaskRunner.  Widget never creates a thread.
    vc::runtime::CameraRunner *m_runner{nullptr};

    bool m_populating_browser{false};
};

#endif // BASLER_CAMERA_WIDGET_H
