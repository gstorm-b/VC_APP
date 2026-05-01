#ifndef MC_PROTOCOL_DEVICE_WIDGET_H
#define MC_PROTOCOL_DEVICE_WIDGET_H

#include <QWidget>
#include "DockWidget.h"

#include "widgets/qtpropertybrowser/qtpropertymanager.h"
#include "widgets/qtpropertybrowser/qtvariantproperty.h"
#include "widgets/qtpropertybrowser/qttreepropertybrowser.h"

#include "device/idevice.h"
#include "device/device_manager.h"
#include "device/plc/mc_protocol_device.h"

#include "model/mc_device_worker.h"
#include <QStringListModel>
#include <QCompleter>

#include "form/device_widget.h"

namespace Ui {
class McProtocolDeviceWidget;
}

class McProtocolDeviceWidget : public IDeviceWidget {
    Q_OBJECT

public:
    explicit McProtocolDeviceWidget(std::shared_ptr<vc::device::IDevice> dv,
                                 ads::CDockWidget *dock = nullptr,
                                 QWidget *parent = nullptr);
    ~McProtocolDeviceWidget();

    void loadConfigToDevice() override;
    void loadConfigToWidget() override;

private slots:
    void onBtnConnect();
    void saveConfig();
    void refreshConfig();

    void onBtnBitDeviceOn();
    void onBtnBitDeviceOff();
    void onBtnBitDeviceToggle();
    void onBtnWordModify();

    void onConnectionStateChanged(vc::device::ConnectStatus state);
    void onPollingUpdateValue(vc::device::McDeviceMap device_map);

private:
    void init_widget();
    void init_m_devices_table();
    void init_d_devices_table();
    void refreshDeviceMap();
    void update_completer();
    void populateBrowser();

private:
    Ui::McProtocolDeviceWidget *ui;
    std::shared_ptr<vc::device::IDevice> m_device;
    vc::device::McProtocolDevice* m_mc_device;
    ads::CDockWidget *m_dock{nullptr};

    vc::device::McProtocolConfig m_config;
    vc::device::McDeviceMap m_device_map;

    QStringListModel *str_model_m_device;
    QStringListModel *str_model_d_device;

    QStringList m_device_names;
    QStringList d_device_names;

    QCompleter *m_device_completer;
    QCompleter *d_device_completer;

    vc::device::mc::McFrameType current_frame_type;

    vc::runtime::McDeviceWorker *m_worker{nullptr};

    QtVariantPropertyManager *variantManager;
    QtVariantEditorFactory *variantFactory;
    QtTreePropertyBrowser *variantEditor;
};


///
/// necessary value
/// bool
/// int
/// real
/// string

#endif // MC_PROTOCOL_DEVICE_WIDGET_H
