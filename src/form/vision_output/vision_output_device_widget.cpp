#include "vision_output_device_widget.h"
#include "ui_vision_output_device_widget.h"
#include "form/pattern/pattern_theme.h"

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
        QVariant value = gadget->property(prop.name());
        QtVariantProperty *variantProp = addPropertyToBrowser(*meta, prop, value, manager, browser);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }
    }
}

static void populateBrowser_VisionOutput(vc::device::VisionTcpipDeviceCfg *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Output cofiguration"));
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

VisionOutputDeviceWidget::VisionOutputDeviceWidget(std::shared_ptr<vc::device::IDevice> dv,
                                               vc::runtime::VisionOutputRunner *runner,
                                               ads::CDockWidget *dock, QWidget *parent)
    : IDeviceWidget(parent),
    ui(new Ui::VisionOutputDeviceWidget),
    m_device(dv),
    m_dock(dock),
    m_runner(runner)  {

    ui->setupUi(this);
    initWidget();
}

VisionOutputDeviceWidget::~VisionOutputDeviceWidget()
{
    delete ui;
}

QString VisionOutputDeviceWidget::deviceId() {
    return m_device->id();
}

void VisionOutputDeviceWidget::loadConfigToDevice() {
    // do nothing
}

void VisionOutputDeviceWidget::loadConfigToWidget() {
    // do nothing
}

void VisionOutputDeviceWidget::initWidget() {
    initPropertyBrowser();

    if (m_device) {
        m_output_device = static_cast<vc::device::VisionTcpipDevice*>(m_device.get());

        // ── Wire to runner (NOT to device directly) ──────────────────────────
        // The runner forwards device signals onto the GUI thread via
        // QueuedConnection, so it is safe to update widgets from these slots.
        // Widget never owns a QThread.
        if (m_runner) {
            connect(m_runner, &vc::runtime::VisionOutputRunner::connectStatusChanged,
                    this,     &VisionOutputDeviceWidget::onConnectionStateChanged);
        } else {
            LOG_DEV_ERR << "McProtocolDeviceWidget: no runner provided — control disabled";
        }

        m_config = m_output_device->visionTcpipConfig();

        populateBrowser();

        connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
                this, &VisionOutputDeviceWidget::onPropertyValueChanged);
    }


    updateConnectionVisual(m_device && m_device->isDeviceConnected()
                               ? vc::device::ConnectStatus::Connected
                               : vc::device::ConnectStatus::Disconnected);
}

void VisionOutputDeviceWidget::onPropertyValueChanged(QtProperty *property, const QVariant &variant) {
    if (m_populating_browser) {
        return;
    }

    // vc::device::McContext *context = this->m_config.context();
    // vc::device::McMsgItf *cfg = context->msgConfig();

    QString propName = property->propertyName();

    const QMetaObject *idevice_meta = m_device->metaObject();
    // const QMetaObject &meta_context = context->getMetaObject();
    const QMetaObject &meta_cfg = m_config.getMetaObject();

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
                m_variantManager->setValue(property, m_device->name());
            }
        }
        return;
    }

    // output config
    index = meta_cfg.indexOfProperty(propName.toUtf8());
    if (index != -1) {
        QMetaProperty mProp = meta_cfg.property(index);
        mProp.writeOnGadget(&m_config, variant);
        // qDebug() << "Updated context: " << propName << "to" << variant;
        this->saveConfig();
        return;
    }
}

void VisionOutputDeviceWidget::saveConfig() {
    m_output_device->setVisionTcpipConfig(m_config);
}

void VisionOutputDeviceWidget::refreshConfig() {
    if (!m_device) {
        return;
    }
}

void VisionOutputDeviceWidget::onConnectionStateChanged(vc::device::ConnectStatus state) {
    updateConnectionVisual(state);
    switch (state) {
    case vc::device::ConnectStatus::Disconnected:

        break;
    default:
        break;
    }
}

void VisionOutputDeviceWidget::populateBrowser() {
    m_variantEditor->blockSignals(true);
    m_populating_browser = true;

    m_variantManager->clear();

    populateBrowser_Device(m_device.get(), m_variantManager, m_variantEditor);
    populateBrowser_VisionOutput(&m_config, m_variantManager, m_variantEditor);

    m_populating_browser = false;
    m_variantEditor->blockSignals(false);
}

void VisionOutputDeviceWidget::updateConnectionVisual(vc::device::ConnectStatus status) {
    using namespace ptn;
    const bool connected = status == vc::device::ConnectStatus::Connected;
    const char *dotColor = connected ? OK : ERR;
    const char *txtColor = connected ? OK : ERR;
    const QString stateText = connected ? tr("CONNECTED") : tr("DISCONNECTED");

    ui->lbl_conn_dot->setStyleSheet(QString(
                                        "background-color: %1; border-radius: 5px; min-width: 10px; max-width: 10px;"
                                        "min-height: 10px; max-height: 10px;"
                                        ).arg(dotColor));
    ui->lbl_conn_state->setText(stateText);
    ui->lbl_conn_state->setStyleSheet(QString(
                                          "color: %1; font: 700 9pt \"Segoe UI\"; letter-spacing: 1px;"
                                          ).arg(txtColor));

    ui->btn_connect->setText(connected ? tr("Disconnect") : tr("Connect"));
    if (connected) {
        ui->btn_connect->setStyleSheet(QString(
                                           "QPushButton { background-color: #2a1a1a; color: %1; "
                                           "  border: 1px solid rgba(232,64,64,0.27); border-radius: 5px; "
                                           "  padding: 7px 22px; font: 700 10pt \"Segoe UI\"; }"
                                           "QPushButton:hover { border-color: %1; }"
                                           "QPushButton:pressed { background-color: %2; }"
                                           ).arg(ERR, BG));
    } else {
        ui->btn_connect->setStyleSheet(QString(
                                           "QPushButton { background-color: %1; color: white; border: none; "
                                           "  border-radius: 5px; padding: 7px 22px; "
                                           "  font: 700 10pt \"Segoe UI\"; }"
                                           "QPushButton:hover { background-color: #3a9ef5; }"
                                           "QPushButton:pressed { background-color: #1f74c7; }"
                                           ).arg(ACC));
    }
}




