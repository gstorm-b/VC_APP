#include "mc_protocol_device_widget.h"
#include "ui_mc_protocol_device_widget.h"

#include "device/plc/mc_request.h"
#include "device/plc/mc_msg_tcp_client.h"
#include "form/pattern/pattern_theme.h"
#include "logger/app_logger.h"

#include <QSignalBlocker>

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
                                          vc::runtime::PlcRunner *runner,
                                          ads::CDockWidget *dock, QWidget *parent)
    : IDeviceWidget(parent),
    ui(new Ui::McProtocolDeviceWidget),
    m_device(dv),
    m_dock(dock),
    m_runner(runner)  {

    ui->setupUi(this);
    initWidget();
}

McProtocolDeviceWidget::~McProtocolDeviceWidget() {
    delete ui;
}

QString McProtocolDeviceWidget::deviceId() {
    return m_device->id();
}

void McProtocolDeviceWidget::loadConfigToDevice() {
    // do nothing
}

void McProtocolDeviceWidget::loadConfigToWidget() {
    // do nothing
}

void McProtocolDeviceWidget::onPropertyValueChanged(QtProperty *property, const QVariant &variant) {
    if (m_populating_browser) {
        return;
    }

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
                m_variantManager->setValue(property, m_device->name());
            }
        }
        return;
    }

    // mc context
    index = meta_context.indexOfProperty(propName.toUtf8());
    if (index != -1) {
        QMetaProperty mProp = meta_context.property(index);
        mProp.writeOnGadget(context, variant);
        // qDebug() << "Updated context: " << propName << "to" << variant;
        this->saveConfig();
        rebuildMonitorRanges();
        refreshMetaSummary();
        return;
    }

    // interface config
    index = meta_msg.indexOfProperty(propName.toUtf8());
    if (index != -1) {
        QMetaProperty mProp = meta_msg.property(index);
        mProp.writeOnGadget(msg_cfg, variant);
        // qDebug() << "Updated context: " << propName << "to" << variant;
        this->saveConfig();
        populateConnectionFields();
        return;
    }
}

void McProtocolDeviceWidget::onBtnConnect() {
    if (!m_device || !m_runner) {
        return;
    }

    if (!m_device->isDeviceConnected()) {
        // saveConfig();
        m_runner->requestConnect();
    } else {
        m_runner->requestDisconnect();
    }
}

void McProtocolDeviceWidget::saveConfig() {
    rebuildMonitorRanges();
    m_mc_device->setMcProtocolConfig(m_config);
}

void McProtocolDeviceWidget::onConnectionStateChanged(vc::device::ConnectStatus state) {
    updateConnectionVisual(state);
    switch (state) {
    case vc::device::ConnectStatus::Disconnected:
        if (m_monitor_m) m_monitor_m->clearAllStatuses();
        if (m_monitor_d) m_monitor_d->clearAllStatuses();
        break;
    default:
        break;
    }
}

void McProtocolDeviceWidget::onPollingUpdateValue(std::shared_ptr<vc::device::PlcValueMap> device_map) {
    m_device_map = *(static_cast<vc::device::McDeviceMap*>(device_map.get()));
    if (m_monitor_m) {
        const auto &m_map = m_device_map.device_map_m;
        for (auto it = m_map.begin(); it != m_map.end(); ++it) {
            m_monitor_m->setBitState(it->first, it->second);
        }
    }
    if (m_monitor_d) {
        const auto &d_map = m_device_map.device_map_d;
        for (auto it = d_map.begin(); it != d_map.end(); ++it) {
            m_monitor_d->setWordValue(it->first, it->second);
        }
    }
}

void McProtocolDeviceWidget::initWidget() {
    initPropertyBrowser();
    applyTheme();

    if (m_device) {
        m_mc_device = static_cast<vc::device::McProtocolDevice*>(m_device.get());

        // ── Wire to runner (NOT to device directly) ──────────────────────────
        // The runner forwards device signals onto the GUI thread via
        // QueuedConnection, so it is safe to update widgets from these slots.
        // Widget never owns a QThread.
        if (m_runner) {
            connect(m_runner, &vc::runtime::PlcRunner::connectStatusChanged,
                    this,     &McProtocolDeviceWidget::onConnectionStateChanged);
            connect(m_runner, &vc::runtime::PlcRunner::pollingUpdate,
                    this,     &McProtocolDeviceWidget::onPollingUpdateValue);
        } else {
            LOG_DEV_ERR << "McProtocolDeviceWidget: no runner provided — control disabled";
        }

        m_config = m_mc_device->mcProtocolConfig();

        // Build the two stacked monitor widgets and slot them into the .ui
        // splitter containers.
        m_monitor_m = new vc::widgets::DevicesMonitorWidget(
            vc::widgets::DevicesMonitorWidget::Mode::Bit, this);
        m_monitor_d = new vc::widgets::DevicesMonitorWidget(
            vc::widgets::DevicesMonitorWidget::Mode::Word, this);

        if (auto *layM = ui->container_m->layout()) layM->addWidget(m_monitor_m);
        if (auto *layD = ui->container_d->layout()) layD->addWidget(m_monitor_d);

        connect(m_monitor_m, &vc::widgets::DevicesMonitorWidget::bitWriteRequested,
                this,        &McProtocolDeviceWidget::onBitWriteRequested);
        connect(m_monitor_d, &vc::widgets::DevicesMonitorWidget::wordWriteRequested,
                this,        &McProtocolDeviceWidget::onWordWriteRequested);

        rebuildMonitorRanges();
        populateConnectionFields();
        refreshMetaSummary();
        populateBrowser();

        connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
                this, &McProtocolDeviceWidget::onPropertyValueChanged);
    }

    connect(ui->btn_connect, &QPushButton::clicked,
            this, &McProtocolDeviceWidget::onBtnConnect);
    connect(ui->ledit_ip, &QLineEdit::editingFinished,
            this, &McProtocolDeviceWidget::onIpEditFinished);
    connect(ui->spb_port, &QSpinBox::editingFinished,
            this, &McProtocolDeviceWidget::onPortEditFinished);
    connect(ui->spb_conn_timeout, &QSpinBox::editingFinished,
            this, &McProtocolDeviceWidget::onConnectTimeoutEditFinished);
    connect(ui->spb_resp_timeout, &QSpinBox::editingFinished,
            this, &McProtocolDeviceWidget::onResponseTimeoutEditFinished);

    updateConnectionVisual(m_device && m_device->isDeviceConnected()
                               ? vc::device::ConnectStatus::Connected
                               : vc::device::ConnectStatus::Disconnected);
}

void McProtocolDeviceWidget::applyTheme() {
    using namespace ptn;

    // Root surface
    this->setStyleSheet(QString(
        "QWidget#McProtocolDeviceWidget { background-color: %1; }"
    ).arg(BG));

    // Connection card
    ui->frame_connection->setStyleSheet(QString(
        "QFrame#frame_connection { background-color: %1; "
        "  border: 1px solid %2; border-radius: 8px; }"
    ).arg(SURF, BD));

    ui->lbl_conn_title->setStyleSheet(QString(
        "color: %1; font: 700 9pt \"Segoe UI\"; letter-spacing: 1.4px;"
    ).arg(TXT3));

    ui->lbl_meta_frame->setStyleSheet(QString(
        "color: %1; font: 9pt \"%2\"; letter-spacing: 0.8px;"
    ).arg(TXT3, FONT_MONO));

    ui->lbl_meta_hint->setStyleSheet(QString(
        "color: %1; font: italic 9pt \"Segoe UI\";"
    ).arg(TXT4));

    // Field labels
    const QString labelStyle = QString(
        "color: %1; font: 700 8pt \"Segoe UI\"; "
        "letter-spacing: 1px; text-transform: uppercase;"
    ).arg(TXT2);
    for (auto *lbl : {ui->lbl_ip, ui->lbl_port,
                       ui->lbl_conn_timeout, ui->lbl_resp_timeout}) {
        lbl->setStyleSheet(labelStyle);
    }

    // Inputs
    const QString inputStyle = QString(
        "QLineEdit, QSpinBox { background-color: %1; color: %2; "
        "  border: 1px solid %3; border-radius: 5px; "
        "  padding: 6px 10px; font: 11pt \"%4\"; min-height: 22px; }"
        "QLineEdit:focus, QSpinBox:focus { border-color: %5; }"
    ).arg(BG, TXT, BD2, FONT_MONO, ACC);
    ui->ledit_ip->setStyleSheet(inputStyle);
    ui->spb_port->setStyleSheet(inputStyle);
    ui->spb_conn_timeout->setStyleSheet(inputStyle);
    ui->spb_resp_timeout->setStyleSheet(inputStyle);

    // Connection status pill
    ui->lbl_conn_dot->setStyleSheet(QString(
        "background-color: %1; border-radius: 5px; min-width: 10px; max-width: 10px;"
        "min-height: 10px; max-height: 10px;"
    ).arg(ERR));
    ui->lbl_conn_state->setStyleSheet(QString(
        "color: %1; font: 700 9pt \"Segoe UI\"; letter-spacing: 1px;"
    ).arg(ERR));

    // Connect button (default: ready-to-connect — primary blue)
    ui->btn_connect->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; border: none; "
        "  border-radius: 5px; padding: 7px 22px; "
        "  font: 700 10pt \"Segoe UI\"; }"
        "QPushButton:hover { background-color: #3a9ef5; }"
        "QPushButton:pressed { background-color: #1f74c7; }"
    ).arg(ACC));

    // Splitter handle
    ui->splitter_device_table->setStyleSheet(QString(
        "QSplitter::handle { background-color: %1; }"
        "QSplitter::handle:hover { background-color: %2; }"
    ).arg(BG, ACC));
}

void McProtocolDeviceWidget::rebuildMonitorRanges() {
    if (!m_config.context()) return;
    const int m_start = m_config.context()->startMAddress();
    const int m_amount = m_config.context()->amountMAddress();
    const int d_start = m_config.context()->startDAddress();
    const int d_amount = m_config.context()->amountDAddress();
    if (m_monitor_m) m_monitor_m->setRange(m_start, m_amount);
    if (m_monitor_d) m_monitor_d->setRange(d_start, d_amount);
}

void McProtocolDeviceWidget::refreshMetaSummary() {
    auto *ctx = m_config.context();
    if (!ctx) return;
    const QString frame = vc::device::mc::McFrameTypeToString(ctx->frameType());
    const QString code  = vc::device::mc::McDataCodeToString(ctx->dataCode());
    ui->lbl_meta_frame->setText(QString("FRAME %1 · %2 · INTERVAL %3 ms")
                                    .arg(frame.isEmpty() ? QStringLiteral("—") : frame.toUpper())
                                    .arg(code.isEmpty()  ? QStringLiteral("—") : code.toUpper())
                                    .arg(ctx->refreshInterval()));
}

void McProtocolDeviceWidget::populateConnectionFields() {
    if (!m_config.context()) return;
    auto *msg = m_config.context()->msgConfig();
    if (!msg) return;

    m_loading_connection_fields = true;
    QSignalBlocker bIp(ui->ledit_ip);
    QSignalBlocker bPort(ui->spb_port);
    QSignalBlocker bConn(ui->spb_conn_timeout);
    QSignalBlocker bResp(ui->spb_resp_timeout);

    if (msg->type() == vc::device::mc::McMsgItfType::EthernetTCPIP) {
        auto *eth = static_cast<vc::device::McMsgEthernetTcpCfg *>(msg);
        ui->ledit_ip->setText(eth->m_ipAddress);
        ui->spb_port->setValue(eth->m_portNumber);
    } else {
        ui->ledit_ip->setText(QString());
        ui->ledit_ip->setEnabled(false);
        ui->spb_port->setEnabled(false);
    }
    ui->spb_conn_timeout->setValue(msg->m_connectTimeout);
    ui->spb_resp_timeout->setValue(msg->m_responseTimeout);

    m_loading_connection_fields = false;
}

void McProtocolDeviceWidget::onIpEditFinished() {
    if (m_loading_connection_fields) return;
    auto *msg = m_config.context() ? m_config.context()->msgConfig() : nullptr;
    if (!msg || msg->type() != vc::device::mc::McMsgItfType::EthernetTCPIP) return;
    auto *eth = static_cast<vc::device::McMsgEthernetTcpCfg *>(msg);
    const QString newIp = ui->ledit_ip->text().trimmed();
    if (newIp == eth->m_ipAddress) return;
    eth->m_ipAddress = newIp;
    saveConfig();
}

void McProtocolDeviceWidget::onPortEditFinished() {
    if (m_loading_connection_fields) return;
    auto *msg = m_config.context() ? m_config.context()->msgConfig() : nullptr;
    if (!msg || msg->type() != vc::device::mc::McMsgItfType::EthernetTCPIP) return;
    auto *eth = static_cast<vc::device::McMsgEthernetTcpCfg *>(msg);
    const int newPort = ui->spb_port->value();
    if (newPort == eth->m_portNumber) return;
    eth->m_portNumber = newPort;
    saveConfig();
}

void McProtocolDeviceWidget::onConnectTimeoutEditFinished() {
    if (m_loading_connection_fields) return;
    auto *msg = m_config.context() ? m_config.context()->msgConfig() : nullptr;
    if (!msg) return;
    const int v = ui->spb_conn_timeout->value();
    if (v == msg->m_connectTimeout) return;
    msg->SetConnectTimeout(v);
    saveConfig();
}

void McProtocolDeviceWidget::onResponseTimeoutEditFinished() {
    if (m_loading_connection_fields) return;
    auto *msg = m_config.context() ? m_config.context()->msgConfig() : nullptr;
    if (!msg) return;
    const int v = ui->spb_resp_timeout->value();
    if (v == msg->m_responseTimeout) return;
    msg->SetWaitResponseTimeout(v);
    saveConfig();
}

void McProtocolDeviceWidget::updateConnectionVisual(vc::device::ConnectStatus status) {
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

void McProtocolDeviceWidget::refreshConfig() {
    if (!m_device) {
        return;
    }
}

void McProtocolDeviceWidget::onBitWriteRequested(int address, quint8 value) {
    if (!m_device || !m_device->isDeviceConnected()) {
        return;
    }
    const QString name = QString("M%1").arg(address, 4, 10, QChar('0'));
    vc::device::MCRequest request(vc::device::MCRequest::WriteBit, name, 1);
    request.buildWriteData_Bit_Device(value ? 0x01 : 0x00);
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::onWordWriteRequested(int address, qint16 value) {
    if (!m_device || !m_device->isDeviceConnected()) {
        return;
    }
    const QString name = QString("D%1").arg(address, 4, 10, QChar('0'));
    vc::device::MCRequest request(vc::device::MCRequest::WriteWord, name, 1);
    request.buildWriteData_Word_Device_Word(static_cast<quint16>(value));
    m_device->pushRequest(static_cast<vc::device::IRequest*>(&request));
}

void McProtocolDeviceWidget::populateBrowser() {
    m_variantEditor->blockSignals(true);
    m_populating_browser = true;

    m_variantManager->clear();

    populateBrowser_Device(m_device.get(), m_variantManager, m_variantEditor);
    populateBrowser_McContext(m_config.context(), m_variantManager, m_variantEditor);
    populateBrowser_MsgInterface(m_config.context()->msgConfig(), m_variantManager, m_variantEditor);

    m_populating_browser = false;
    m_variantEditor->blockSignals(false);
}
