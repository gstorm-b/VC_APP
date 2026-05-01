#ifndef ADD_DEVICE_WIZARD_H
#define ADD_DEVICE_WIZARD_H

#include <QDialog>
#include <QFrame>
#include <QPushButton>
#include <QLineEdit>
#include <QComboBox>
#include <QMap>
#include "device/device_manager.h"

class AddDeviceWizard : public QDialog {
    Q_OBJECT

public:
    explicit AddDeviceWizard(std::shared_ptr<vc::device::DeviceManager> mng,
                             const QString &taskName = QString(),
                             QWidget *parent = nullptr);

    int showWizard();

    QString getDeviceId()   const { return m_pendingDeviceId; }
    QString getDeviceName() const;
    QString getDeviceType() const;

    void reject() override;

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override;
    void keyPressEvent(QKeyEvent *event) override;

private slots:
    void onCardClicked(vc::device::DeviceType type);
    void onAddClicked();
    void onNameChanged(const QString &text);

private:
    void buildUi(const QString &taskName);
    void selectCard(vc::device::DeviceType type);
    QJsonObject buildDeviceJson(vc::device::DeviceType type);

    std::shared_ptr<vc::device::DeviceManager> m_manager;
    QString                    m_pendingDeviceId;
    vc::device::DeviceType     m_selectedType{vc::device::DeviceType::Camera};

    QMap<vc::device::DeviceType, QFrame *>  m_cards;
    QMap<vc::device::DeviceType, QString>   m_defaultNames;
    QMap<vc::device::DeviceType, QColor>    m_cardColors;

    QLineEdit   *m_nameEdit{nullptr};
    QPushButton *m_addBtn{nullptr};

    // Hidden widgets for type-specific config defaults
    QComboBox *m_cbxCameraType{nullptr};
    QComboBox *m_cbxMcFrameType{nullptr};
    QComboBox *m_cbxMcCode{nullptr};
};

#endif // ADD_DEVICE_WIZARD_H
