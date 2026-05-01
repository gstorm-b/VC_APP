#include "add_device_wizard.h"
#include "ui_add_device_wizard.h"

#include <QKeyEvent>
#include <QMessageBox>

#include "logger/app_logger.h"
#include "device/device_factory.h"
#include "device/camera/camera_device.h"
#include "device/plc/mc_protocol_device.h"

AddDeviceWizard::AddDeviceWizard(std::shared_ptr<vc::device::DeviceManager> mng, QWidget *parent)
    : QDialog(parent),
    ui(new Ui::AddDeviceWizard),
    manager(mng) {

    ui->setupUi(this);

    setWindowTitle(tr("Add New Device Wizard"));
    setMinimumWidth(450);

    if (!mng) {
        LOG_USER_ERR << tr("System error, cannot initialize Add Device Wizard (Input Device Manager is null).");
        return;
    }


    ui->ledit_dv_name->setPlaceholderText(tr("Input device name..."));
    ui->ledit_dv_id->setReadOnly(true);

    ui->cbx_dv_type->addItems(manager->getDeviceTypeList());

    ui->config_stack_wg->addWidget(createCameraConfigWidget());  // Index 0
    ui->config_stack_wg->addWidget(createMcDeviceConfigWidget());     // Index 1

    connect(ui->ledit_dv_name, &QLineEdit::textChanged, this, &AddDeviceWizard::onDeviceNameTextChanged);
    connect(ui->cbx_dv_type, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &AddDeviceWizard::onDeviceTypeChanged);
    connect(ui->config_stack_wg, &QStackedWidget::currentChanged,
            this, &AddDeviceWizard::onStackWidgetChanged);

    connect(ui->btn_finish, &QPushButton::clicked, this, &AddDeviceWizard::onFinishClicked);
    connect(ui->btn_cancel, &QPushButton::clicked, this, &AddDeviceWizard::reject);

    // update stack widget for first widget
    onDeviceTypeChanged(0);
}

AddDeviceWizard::~AddDeviceWizard() {
    delete ui;
}

int AddDeviceWizard::showWizard() {
    pendingDeviceId = manager->allocatePendingId();
    ui->ledit_dv_id->setText(pendingDeviceId);
    ui->ledit_dv_name->setFocus();
    return this->exec();
}

QString AddDeviceWizard::getDeviceId() const {
    return ui->ledit_dv_id->text();
}

QString AddDeviceWizard::getDeviceName() const {
    return ui->ledit_dv_name->text();
}

QString AddDeviceWizard::getDeviceType() const {
    return ui->cbx_dv_type->currentText();
}

void AddDeviceWizard::reject() {
    // manager->releaseDevice(pendingDeviceId);
    manager->releasePendingId(pendingDeviceId);
    QDialog::reject();
}

void AddDeviceWizard::onDeviceTypeChanged(int index) {
    ui->config_stack_wg->setCurrentIndex(index);
}

void AddDeviceWizard::onStackWidgetChanged(int index) {
    if (index == 0) {
        return;
    } else if (index == 1) {
        McFrameTypeChanged(cbx_mc_frame_type->currentText());
    }
}

void AddDeviceWizard::McFrameTypeChanged(QString text) {
    vc::device::mc::McFrameType frame_type = vc::device::mc::McFrameTypeFromString(text);
    QString interface_text = tr("Unknown");
    if ((frame_type == vc::device::mc::McFrameType::Frame_1E) or
        (frame_type == vc::device::mc::McFrameType::Frame_3E)) {
        interface_text = vc::device::mc::McMsgItfTypeToString(vc::device::mc::McMsgItfType::EthernetTCPIP);
    } else if ((frame_type == vc::device::mc::McFrameType::Frame_1C) or
               (frame_type == vc::device::mc::McFrameType::Frame_3C)) {
        interface_text = vc::device::mc::McMsgItfTypeToString(vc::device::mc::McMsgItfType::SerialPort);
    }

    lb_mc_interface->setText(interface_text);
}

void AddDeviceWizard::onFinishClicked() {
    QString inputName = ui->ledit_dv_name->text().trimmed();

    // validate data before close
    if (inputName.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Add device warning"),
                             tr("Please input device name."));
        ui->ledit_dv_name->setFocus();
        return;
    }

    if (manager->isNameExists(inputName)) {
        QMessageBox::warning(this,
                             tr("Add device warning"),
                             tr("Device name already existed."));
        ui->ledit_dv_name->selectAll();
        return;
    }

    vc::device::DeviceType type = vc::device::DeviceTypeFromString(ui->cbx_dv_type->currentText());
    QJsonObject jobj = createDeviceJsonObject(type);
    jobj[DEVICE_JSK_ID] = pendingDeviceId;
    jobj[DEVICE_JSK_NAME] = inputName;
    jobj[DEVICE_JSK_TYPE] = vc::device::DeviceTypeToString(type);
    std::shared_ptr<vc::device::IDevice> dv(vc::device::DeviceFactory::create(type, jobj));

    if (manager->commitDevice(pendingDeviceId, inputName, dv)) {
        accept();
    } else {
        QMessageBox::critical(this,
                              tr("Add device warning"),
                              tr("Cannot register with this ID, please turn off wizard and try again."));
    }

    // accept();
}

void AddDeviceWizard::onDeviceNameTextChanged(const QString &text) {
    if (text.trimmed().isEmpty()) {
        ui->btn_cancel->setDefault(true);
        ui->btn_finish->setDefault(false);
    } else {
        ui->btn_cancel->setDefault(false);
        ui->btn_finish->setDefault(true);
    }
}

void AddDeviceWizard::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        event->ignore();
    } else {
        QDialog::keyPressEvent(event);
    }
}

QJsonObject AddDeviceWizard::createDeviceJsonObject(vc::device::DeviceType type) {
    QJsonObject obj;

    switch (type) {
    case vc::device::DeviceType::UserType:
        return obj;
    case vc::device::DeviceType::Camera:
        obj[DEVICE_JSK_CAM_TYPE] = cbx_camera_type->currentText();
        break;
    case vc::device::McDevice:
    {
        vc::device::McProtocolConfig config;
        vc::device::mc::McFrameType frame_type = vc::device::mc::McFrameTypeFromString(cbx_mc_frame_type->currentText());
        vc::device::mc::McDataCode data_code = vc::device::mc::McDataCodeFromString(cbx_mc_code->currentText());
        config.configMcProtocol(frame_type, data_code);
        obj[DEVICE_JSK_CONFIG] = config.toJson();
    }
        break;
    case vc::device::TCPIP_DEVICE:
        break;
    case vc::device::Robot:
        break;
    default:
        break;
    }

    return obj;
}

QWidget* AddDeviceWizard::createCameraConfigWidget() {
    auto *widget = new QWidget();
    auto *layout = new QFormLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    cbx_camera_type = new QComboBox();
    cbx_camera_type->addItems(manager->getSubDeviceTypeList(vc::device::Camera));

    layout->addRow(tr("Camera Type:"), cbx_camera_type);
    // layout->addRow(tr("Interface"), cbx_camera_interface);

    return widget;
}

QWidget* AddDeviceWizard::createMcDeviceConfigWidget() {
    auto *widget = new QWidget();
    auto *layout = new QFormLayout(widget);
    layout->setContentsMargins(0, 0, 0, 0);

    QLabel *mc_label = new QLabel("MC Protocol");
    lb_mc_interface = new QLabel("");

    cbx_mc_frame_type = new QComboBox();
    cbx_mc_frame_type->addItems(manager->getSubDeviceTypeList(vc::device::McDevice));

    cbx_mc_code = new QComboBox();
    cbx_mc_code->addItem(vc::device::mc::McDataCodeToString(vc::device::mc::McDataCode::Binary));
    // cbx_mc_code->addItem(vc::device::McDataCodeToString(vc::device::McDataCode::Ascii));

    layout->addRow(tr("Protocol"), mc_label);
    layout->addRow(tr("Frame type"), cbx_mc_frame_type);
    layout->addRow(tr("Data code"), cbx_mc_code);
    layout->addRow(tr("Interface"), lb_mc_interface);

    connect(cbx_mc_frame_type, &QComboBox::currentTextChanged,
            this, &AddDeviceWizard::McFrameTypeChanged);

    return widget;
}




