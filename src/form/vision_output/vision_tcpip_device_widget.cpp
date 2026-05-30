#include "vision_tcpip_device_widget.h"
#include "ui_vision_tcpip_device_widget.h"

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

static void populateBrowser_Gadget(const QMetaObject &meta,
                                   const void *gadget,
                                   const QString &groupName,
                                   QtVariantPropertyManager *manager,
                                   QtTreePropertyBrowser *browser)
{
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(), groupName);
    browser->addProperty(topItem);

    const int propCount = meta.propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta.property(i);
        QVariant value = prop.readOnGadget(gadget);
        QtVariantProperty *variantProp = addPropertyToBrowser(meta, prop, value, manager, browser);
        if (!variantProp) {
            continue;
        }

        variantProp->setEnabled(false);
        topItem->addSubProperty(variantProp);
    }
}

VisionTcpipDeviceWidget::VisionTcpipDeviceWidget(std::shared_ptr<vc::device::IDevice> dv,
                                               vc::runtime::VisionOutputRunner *runner,
                                               ads::CDockWidget *dock, QWidget *parent)
    : IDeviceWidget(parent),
    ui(new Ui::VisionTcpipDeviceWidget),
    m_device(dv),
    m_dock(dock),
    m_runner(runner)  {

    ui->setupUi(this);
    initWidget();
}

VisionTcpipDeviceWidget::~VisionTcpipDeviceWidget()
{
    delete ui;
}

QString VisionTcpipDeviceWidget::deviceId() {
    return m_device->id();
}

void VisionTcpipDeviceWidget::loadConfigToDevice() {
    // do nothing
}

void VisionTcpipDeviceWidget::loadConfigToWidget() {
    // do nothing
}

void VisionTcpipDeviceWidget::initWidget() {
    initPropertyBrowser();
    setupThemeReload(QStringLiteral(":/styles/vision_tcpip_device_widget_dark.qss"),
                    QStringLiteral(":/styles/vision_tcpip_device_widget_light.qss"));

    if (m_device) {
        m_output_device = static_cast<vc::device::VisionTcpipDevice*>(m_device.get());

        // ── Wire to runner (NOT to device directly) ──────────────────────────
        // The runner forwards device signals onto the GUI thread via
        // QueuedConnection, so it is safe to update widgets from these slots.
        // Widget never owns a QThread.
        if (m_runner) {
            connect(m_runner, &vc::runtime::VisionOutputRunner::connectStatusChanged,
                    this,     &VisionTcpipDeviceWidget::onConnectionStateChanged);
        } else {
            LOG_DEV_ERR << "VisionTcpipDeviceWidget: no runner provided - control disabled";
        }

        m_config = m_output_device->visionTcpipConfig();

        populateBrowser();

        connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
                this, &VisionTcpipDeviceWidget::onPropertyValueChanged);
    }


    updateConnectionVisual(m_device && m_device->isDeviceConnected()
                               ? vc::device::ConnectStatus::Connected
                               : vc::device::ConnectStatus::Disconnected);
}

void VisionTcpipDeviceWidget::onPropertyValueChanged(QtProperty *property, const QVariant &variant) {
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

void VisionTcpipDeviceWidget::saveConfig() {
    m_output_device->setVisionTcpipConfig(m_config);
}

void VisionTcpipDeviceWidget::refreshConfig() {
    if (!m_device) {
        return;
    }
}

void VisionTcpipDeviceWidget::onConnectionStateChanged(vc::device::ConnectStatus state) {
    updateConnectionVisual(state);
    populateBrowser();
    switch (state) {
    case vc::device::ConnectStatus::Disconnected:

        break;
    default:
        break;
    }
}

void VisionTcpipDeviceWidget::populateBrowser() {
    m_variantEditor->blockSignals(true);
    m_populating_browser = true;

    m_variantManager->clear();

    populateBrowser_Device(m_device.get(), m_variantManager, m_variantEditor);
    populateBrowser_VisionOutput(&m_config, m_variantManager, m_variantEditor);
    if (m_output_device) {
        const vc::device::VisionTcpipRuntimeState runtimeState = m_output_device->runtimeState();
        const vc::device::VisionTcpipDiagnostics diagnostics = m_output_device->diagnostics();
        populateBrowser_Gadget(vc::device::VisionTcpipRuntimeState::staticMetaObject,
                               &runtimeState,
                               QLatin1String("Runtime State"),
                               m_variantManager,
                               m_variantEditor);
        populateBrowser_Gadget(vc::device::VisionTcpipDiagnostics::staticMetaObject,
                               &diagnostics,
                               QLatin1String("Diagnostics"),
                               m_variantManager,
                               m_variantEditor);
    }

    m_populating_browser = false;
    m_variantEditor->blockSignals(false);
}

void VisionTcpipDeviceWidget::updateConnectionVisual(vc::device::ConnectStatus status) {
    const bool connected = status == vc::device::ConnectStatus::Connected;
    const QByteArray state = connected ? "connected" : "disconnected";

    ui->lbl_conn_state->setText(connected ? tr("CONNECTED") : tr("DISCONNECTED"));
    ui->btn_connect->setText(connected ? tr("Disconnect") : tr("Connect"));

    for (QWidget *w : {static_cast<QWidget*>(ui->lbl_conn_dot),
                       static_cast<QWidget*>(ui->lbl_conn_state),
                       static_cast<QWidget*>(ui->btn_connect)}) {
        w->setProperty("connectionState", state);
        w->style()->unpolish(w);
        w->style()->polish(w);
        w->update();
    }
}




