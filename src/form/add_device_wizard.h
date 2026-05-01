#ifndef ADD_DEVICE_WIZARD_H
#define ADD_DEVICE_WIZARD_H

#include <QDialog>
#include <QComboBox>
#include <QLabel>
#include "device/device_manager.h"

namespace Ui {
class AddDeviceWizard;
}

class AddDeviceWizard : public QDialog
{
    Q_OBJECT

public:
    explicit AddDeviceWizard(std::shared_ptr<vc::device::DeviceManager> mng, QWidget *parent = nullptr);
    ~AddDeviceWizard();

    int showWizard();

    QString getDeviceId() const;
    QString getDeviceName() const;
    QString getDeviceType() const;

    void reject() override;

private slots:
    void onDeviceTypeChanged(int index);
    void onFinishClicked();
    void onDeviceNameTextChanged(const QString &text);
    void onStackWidgetChanged(int index);

    void McFrameTypeChanged(QString text);

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    QJsonObject createDeviceJsonObject(vc::device::DeviceType type);
    QWidget* createCameraConfigWidget();
    QWidget* createMcDeviceConfigWidget();
    QWidget* createOutputConfigWidget();

private:
    Ui::AddDeviceWizard *ui;
    std::shared_ptr<vc::device::DeviceManager> manager;
    QString pendingDeviceId;

    QComboBox *cbx_camera_type{nullptr};
    QComboBox *cbx_camera_interface{nullptr};

    QComboBox *cbx_mc_frame_type{nullptr};
    QLabel *lb_mc_interface{nullptr};
    QComboBox *cbx_mc_code{nullptr};
};

#endif // ADD_DEVICE_WIZARD_H
