#include "mc_protocol_device_widget.h"
#include "ui_mc_protocol_device_widget.h"
// #include "device/plc/mc_msg_tcp_client.h"
#include "device/plc/mc_request.h"

// hepler function
static QtVariantProperty* addPropertyToBrowser(const QMetaObject &meta, QMetaProperty &prop, QVariant &value,
                                        QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {

    QString propName = prop.name();
    if (propName == "objectName") {
        // ignore objectName
        return nullptr;
    }

    QtVariantProperty *variantProp = nullptr;
    // enum type Check
    if (prop.isEnumType()) {
        variantProp = manager->addProperty(QtVariantPropertyManager::enumTypeId(), propName);

        // get enum names
        QStringList enumNames;
        QMetaEnum metaEnum = prop.enumerator();
        for (int j = 0; j < metaEnum.keyCount(); ++j) {
            const char* key = metaEnum.key(j);
            QString translatedName = QCoreApplication::translate(meta.className(), key);
            enumNames << translatedName;
        }
        variantProp->setAttribute(QLatin1String("enumNames"), enumNames);
        // set enum value
        variantProp->setValue(value.toInt());

    } else {
        // normal data types handle (int, QString, bool, QColor, v.v.)
        int typeId = value.userType();
        variantProp = manager->addProperty(typeId, propName);
        if (variantProp) {
            variantProp->setValue(value);
        }
    }

    if (!variantProp) {
        return nullptr;
    }

    int displayNameIdx = meta.indexOfClassInfo(QString("%1_name").arg(propName).toUtf8());
    if (displayNameIdx != -1) {
        const QString displayName = meta.classInfo(displayNameIdx).value();
        if (!displayName.isEmpty()) {
            variantProp->setDisplayName(displayName);
        }
    }

    // --- set attributes---
    int minIdx = meta.indexOfClassInfo(QString("%1_min").arg(propName).toUtf8());
    if (minIdx != -1) {
        variantProp->setAttribute("minimum", QString(meta.classInfo(minIdx).value()).toInt());
    }

    int maxIdx = meta.indexOfClassInfo(QString("%1_max").arg(propName).toUtf8());
    if (maxIdx != -1) {
        variantProp->setAttribute("maximum", QString(meta.classInfo(maxIdx).value()).toInt());
    }

    if (!prop.isWritable()) {
        variantProp->setEnabled(false);
    }

    return variantProp;
}

static void populateBrowser_Device(vc::device::IDevice *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Device Information"));
    browser->addProperty(topItem);
    const QMetaObject *meta = gadget->metaObject();

    int propCount = meta->propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta->property(i);
        // QVariant value = prop.readOnGadget(gadget);
        QVariant value = gadget->property(prop.name());
        QtVariantProperty *variantProp = addPropertyToBrowser(*meta, prop, value, manager, browser);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }
    }
}


static void populateBrowser_MsgInterface(vc::device::McMsgItfConfig *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Interface"));
    browser->addProperty(topItem);
    const QMetaObject &meta = gadget->getMetaObject();

    int propCount = meta.propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta.property(i);
        QVariant value = prop.readOnGadget(gadget);

        QtVariantProperty *variantProp = addPropertyToBrowser(meta, prop, value, manager, browser);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }
    }
}

static void populateBrowser_McContext(vc::device::McContext *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Protocol format"));
    browser->addProperty(topItem);
    const QMetaObject &meta = gadget->getMetaObject();

    int propCount = meta.propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta.property(i);
        QVariant value = prop.readOnGadget(gadget);

        QtVariantProperty *variantProp = addPropertyToBrowser(meta, prop, value, manager, browser);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }
    }
}

McProtocolDeviceWidget::McProtocolDeviceWidget(std::shared_ptr<vc::device::IDevice> dv,
                                          ads::CDockWidget *dock, QWidget *parent)
    : IDeviceWidget(parent),
    ui(new Ui::McProtocolDeviceWidget),
    m_device(dv),
    m_dock(dock)  {

    ui->setupUi(this);

    init_widget();

    if (m_device) {
        m_mc_device = static_cast<vc::device::McProtocolDevice*>(m_device.get());
        connect(m_mc_device, &vc::device::McProtocolDevice::pollingUpdate,
                this, &McProtocolDeviceWidget::onPollingUpdateValue);

        m_worker = new vc::runtime::McDeviceWorker(m_mc_device);
        m_worker->moveToWorker();

        str_model_m_device = new QStringListModel(m_device_names, this);
        str_model_d_device = new QStringListModel(d_device_names, this);

        m_device_completer = new QCompleter(this);
        m_device_completer->setCaseSensitivity(Qt::CaseInsensitive);
        d_device_completer = new QCompleter(this);
        d_device_completer->setCaseSensitivity(Qt::CaseInsensitive);

        m_device_completer->setModel(str_model_m_device);
        d_device_completer->setModel(str_model_d_device);

        ui->ledit_bit_address->setCompleter(m_device_completer);
        ui->ledit_word_address->setCompleter(d_device_completer);

        m_config = m_mc_device->mcProtocolConfig();
        populateBrowser();

        connect(variantManager, &QtVariantPropertyManager::valueChanged, this, [=](QtProperty *property, const QVariant &variant) {
            vc::device::McContext *context = this->m_config.context();
            vc::device::McMsgItfConfig *msg_cfg = context->msgConfig();

            QString propName = property->propertyName();

            const QMetaObject *idevice_meta = m_device->metaObject();
            const QMetaObject &meta_context = context->getMetaObject();
            const QMetaObject &meta_msg = msg_cfg->getMetaObject();

            // abstract device
            int index = idevice_meta->indexOfProperty(propName.toUtf8());
            if (index != -1) {
                if (propName == "name") {
                    QString new_name = variant.toString();
                    // avoid loop made by signal valueChanged
                    if (m_device->name() == new_name) {
                        return;
                    }

                    if (!m_device->deviceManager()->changeDeviceName(m_device->id(), new_name)) {
                        variantManager->setValue(property, m_device->name());
                    }
                }
                return;
            }

            // mc context
            index = meta_context.indexOfProperty(propName.toUtf8());
            if (index != -1) {
                QMetaProperty mProp = meta_context.property(index);
                mProp.writeOnGadget(context, variant);
                qDebug() << "Updated context: " << propName << "to" << variant;
                this->saveConfig();
                return;
            }

            // interface config
            index = meta_msg.indexOfProperty(propName.toUtf8());
            if (index != -1) {
                QMetaProperty mProp = meta_msg.property(index);
                mProp.writeOnGadget(msg_cfg, variant);
                qDebug() << "Updated context: " << propName << "to" << variant;
                this->saveConfig();
                return;
            }
        });

        refreshDeviceMap();
    }
}

McProtocolDeviceWidget::~McProtocolDeviceWidget() {
    delete ui;
}

void McProtocolDeviceWidget::loadConfigToDevice() {

}

void McProtocolDeviceWidget::loadConfigToWidget() {

}

void McProtocolDeviceWidget::onBtnConnect() {
    if (!m_device) {
        return;
    }

    if (!m_device->isDeviceConnected()) {
        // saveConfig();
        m_worker->connectMcDevice();
    } else {
        m_worker->disconnectMcDevice();
    }
}

void McProtocolDeviceWidget::saveConfig() {
    refreshDeviceMap();
    m_mc_device->setMcProtocolConfig(m_config);
}

void McProtocolDeviceWidget::onConnectionStateChanged(vc::device::ConnectStatus state) {
    switch (state) {
    // case vc::device::ConnectStatus::Connected:
    //     ui->table_m_device->clearStatusAllDevices();
    //     ui->table_d_device->clearStatusAllDevices();
    //     break;

    case vc::device::ConnectStatus::Disconnected:
        ui->table_m_device->clearStatusAllDevices();
        ui->table_d_device->clearStatusAllDevices();
        break;
    default:
        break;
    }
}

void McProtocolDeviceWidget::onPollingUpdateValue(vc::device::McDeviceMap device_map) {
    m_device_map = device_map;
    std::map<int, quint8> &m_devices_map = device_map.device_map_m;
    std::map<int, quint8>::const_iterator it_m = m_devices_map.begin();
    for (; it_m != m_devices_map.end();++it_m) {
        QString device_name = QString("M%1").arg(it_m->first, 4, 10, QChar('0'));
        ui->table_m_device->editDeviceStatus(device_name, it_m->second);
    }

    std::map<int, qint16> &d_devices_map = device_map.device_map_d;
    std::map<int, qint16>::const_iterator it_d = d_devices_map.begin();
    for (; it_d != d_devices_map.end();++it_d) {
        QString device_name = QString("D%1").arg(it_d->first, 4, 10, QChar('0'));
        ui->table_d_device->editDeviceValue(device_name, it_d->second);
    }
}

void McProtocolDeviceWidget::init_widget() {

    connect(ui->btn_connect, &QPushButton::clicked,
            this, & McProtocolDeviceWidget::onBtnConnect);
    connect(ui->btn_bit_on, &QPushButton::clicked,
            this, & McProtocolDeviceWidget::onBtnBitDeviceOn);
    connect(ui->btn_bit_off, &QPushButton::clicked,
            this, & McProtocolDeviceWidget::onBtnBitDeviceOff);
    connect(ui->btn_bit_toggle, &QPushButton::clicked,
            this, & McProtocolDeviceWidget::onBtnBitDeviceToggle);
    connect(ui->btn_word_modify, &QPushButton::clicked,
            this, & McProtocolDeviceWidget::onBtnWordModify);
    connect(ui->spb_word_modify, &QSpinBox::editingFinished,
            this, & McProtocolDeviceWidget::onBtnWordModify);

    // init property browser
    variantManager = new QtVariantPropertyManager(this);
    variantFactory = new QtVariantEditorFactory(this);
    variantEditor = new QtTreePropertyBrowser();

    QHBoxLayout *layout = new QHBoxLayout();
    ui->wg_property_browser->setLayout(layout);
    layout->addWidget(variantEditor);
    layout->setContentsMargins(0,0,0,0);

    variantEditor->setAlternatingRowColors(false);
    variantEditor->setFactoryForManager(variantManager, variantFactory);
    variantEditor->setPropertiesWithoutValueMarked(true);
    variantEditor->setRootIsDecorated(false);
    variantEditor->setResizeMode(QtTreePropertyBrowser::Stretch);

    ui->splitter_main->setStretchFactor(0, 7);
    ui->splitter_main->setStretchFactor(0, 3);
}

void McProtocolDeviceWidget::init_m_devices_table() {
    if (!m_device) {
        return;
    }

    m_device_names.clear();
    ui->table_m_device->removeAllRow();

    ui->table_m_device->setCommentEditableMode(true);

    int start = m_config.context()->startMAddress();
    int amount = m_config.context()->amountMAddress();

    for (int idx=0;idx<amount;idx++) {
        QString device_name = QString("M%1").arg(start + idx, 4, 10, QChar('0'));
        m_device_names.push_back(device_name);
        ui->table_m_device->addNewRow(device_name, false, QString(""));
    }
}

void McProtocolDeviceWidget::init_d_devices_table() {
    if (!m_device) {
        return;
    }

    d_device_names.clear();
    ui->table_d_device->removeAllRow();
    ui->table_d_device->setCommentEditableMode(true);

    int start = m_config.context()->startDAddress();
    int amount = m_config.context()->amountDAddress();

    for (int idx=0;idx<amount;idx++) {
        QString device_name = QString("D%1").arg(start + idx, 4, 10, QChar('0'));
        d_device_names.push_back(device_name);
        ui->table_d_device->addNewRow(device_name, false, QString(""));
    }
}

void McProtocolDeviceWidget::refreshConfig() {
    // if (!m_device) {
    //     return;
    // }

}


void McProtocolDeviceWidget::onBtnBitDeviceOn() {
    if (!m_device->isDeviceConnected()) {
        return;
    }

    QString start_address = ui->ledit_bit_address->text();
    if (start_address.isEmpty()) {
        return;
    }

    vc::device::MCRequest request(vc::device::MCRequest::WriteBit, start_address, 1);
    request.buildWriteData_Bit_Device(0x01);
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::onBtnBitDeviceOff() {
    if (!m_device->isDeviceConnected()) {
        return;
    }

    QString start_address = ui->ledit_bit_address->text();
    if (start_address.isEmpty()) {
        return;
    }

    vc::device::MCRequest request(vc::device::MCRequest::WriteBit, start_address, 1);
    request.buildWriteData_Bit_Device(0x00);
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::onBtnBitDeviceToggle() {
    if (!m_device->isDeviceConnected()) {
        return;
    }

    QString start_address = ui->ledit_bit_address->text();
    if (start_address.isEmpty()) {
        return;
    }

    vc::device::MCRequest request(vc::device::MCRequest::WriteBit, start_address, 1);
    quint8 value = m_device_map.device_map_m[request.m_start_address];
    request.buildWriteData_Bit_Device((value == 0x00) ? 0x01 : 0x00);
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::onBtnWordModify() {
    if (!m_device->isDeviceConnected()) {
        return;
    }

    QString start_address = ui->ledit_word_address->text();
    if (start_address.isEmpty()) {
        return;
    }

    vc::device::MCRequest request(vc::device::MCRequest::WriteWord, start_address, 1);
    request.buildWriteData_Word_Device_Word(static_cast<quint16>(ui->spb_word_modify->value()));
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::refreshDeviceMap() {
    init_d_devices_table();
    init_m_devices_table();
    update_completer();
}

void McProtocolDeviceWidget::update_completer() {
    str_model_m_device->setStringList(m_device_names);
    str_model_d_device->setStringList(d_device_names);
}

void McProtocolDeviceWidget::populateBrowser() {
    variantEditor->blockSignals(true);

    variantManager->clear();

    populateBrowser_Device(m_device.get(), variantManager, variantEditor);
    populateBrowser_McContext(m_config.context(), variantManager, variantEditor);
    populateBrowser_MsgInterface(m_config.context()->msgConfig(), variantManager, variantEditor);

    variantEditor->blockSignals(false);
}
