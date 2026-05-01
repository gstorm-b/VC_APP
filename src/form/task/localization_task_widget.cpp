#include "localization_task_widget.h"
#include "ui_localization_task_widget.h"
#include "model/project.h"

#include <QInputDialog>

LocalizationTaskWidget::LocalizationTaskWidget(std::shared_ptr<vc::model::ITask> task,
                                               ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
    ui(new Ui::LocalizationTaskWidget) {

    ui->setupUi(this);
    initWidget();
}

LocalizationTaskWidget::~LocalizationTaskWidget()
{
    delete ui;
}

void LocalizationTaskWidget::loadConfigToTask() {

}

void LocalizationTaskWidget::loadConfigToWidget() {

}

void LocalizationTaskWidget::initWidget() {
    initPropertyBrowser(ui->wg_property_browser);

    ui->splitter_main->setStretchFactor(0, 7);
    ui->splitter_main->setStretchFactor(1, 3);

    if (m_task) {
        m_localizeTask = static_cast<vc::model::TaskLocalization*>(m_task.get());
        m_config =  m_localizeTask->taskLocalizeConfig();
        connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
                this, &LocalizationTaskWidget::onPropertyManagerValueChanged);
    }

    connect(ui->tbtn_select_comm_device, &QToolButton::clicked,
            this, &LocalizationTaskWidget::onSelectCommDeviceClicked);
    connect(ui->tbtn_select_output_device, &QToolButton::clicked,
            this, &LocalizationTaskWidget::onSelectOutputDeviceClicked);

    ui->ledit_comm_device->setReadOnly(true);
    ui->ledit_output_device->setReadOnly(true);

    ui->ledit_comm_device->setPlaceholderText(tr("Select Communication device..."));
    ui->ledit_output_device->setPlaceholderText(tr("Select Output device..."));

    this->updateCameraMappingToWidget();
    connect(m_localizeTask->project()->deviceManager().get(), &vc::device::DeviceManager::devicesChanged, this, [this]() {
        this->ui->camera_mapping_wg->setCameraList(m_localizeTask->project()->deviceManager()->cameraDevicesNameList());
    });

    connect(ui->camera_mapping_wg, &CameraMappingWidget::mappingChanged,
            this, &LocalizationTaskWidget::onCameraMappingChanged);

    updateCommDeviceInfo();
    updateOutputDeviceInfo();
}

void LocalizationTaskWidget::updateCommDeviceInfo() {
    if (!m_localizeTask) {
        return;
    }

    // disconnect current comm device before connect to the new one
    if (m_commDevice) {
        disconnect(m_commDevice.get(), &vc::device::IDevice::configChanged,
                this, &LocalizationTaskWidget::onUpdateCompleter);
    }

    m_commDevice = m_localizeTask->project()->deviceById(m_config.d->m_sCommDeviceId);
    if (m_commDevice) {
        ui->ledit_comm_device->setText(m_commDevice->name());
        connect(m_commDevice.get(), &vc::device::IDevice::configChanged,
                this, &LocalizationTaskWidget::onUpdateCompleter);
        onUpdateCompleter();
    } else {
        populateBrowser();
    }

}

void LocalizationTaskWidget::updateOutputDeviceInfo() {
    if (!m_localizeTask) {
        return;
    }

    std::shared_ptr<vc::device::IDevice> outputDevice =
        m_localizeTask->project()->deviceById(m_config.d->m_sOutputDeviceId);

    if (outputDevice) {
        ui->ledit_output_device->setText(outputDevice->name());
    }
}

void LocalizationTaskWidget::populateBrowser() {
    m_variantEditor->blockSignals(true);
    m_variantManager->clear();
    m_populating_browser = true;

    populateBrowser_Task();
    populateBrowser_Config();

    m_populating_browser = false;
    m_variantEditor->blockSignals(false);
}

void LocalizationTaskWidget::populateBrowser_Task() {
    QtProperty *topItem = m_variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Task information"));
    m_variantEditor->addProperty(topItem);
    const QMetaObject *meta = m_localizeTask->metaObject();

    int propCount = meta->propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta->property(i);
        QVariant value = m_localizeTask->property(prop.name());
        QtVariantProperty *variantProp = LocalizationTaskWidget::addPropertyToBrowser(*meta, prop, value, m_variantManager, m_variantEditor);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }
    }
}

void LocalizationTaskWidget::populateBrowser_Config() {
    QtProperty *topItem = m_variantManager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Configuration"));
    m_variantEditor->addProperty(topItem);
    const QMetaObject &meta = m_config.getMetaObject();

    int propCount = meta.propertyCount();
    for (int i = 0; i < propCount; ++i) {
        QMetaProperty prop = meta.property(i);
        QVariant value = prop.readOnGadget(&m_config);

        QtVariantProperty *variantProp = LocalizationTaskWidget::addPropertyToBrowser(meta, prop, value, m_variantManager, m_variantEditor);
        if (variantProp) {
            topItem->addSubProperty(variantProp);
        }

        QString propName = prop.name();
        if (propName.first(1) == "b") {
            variantProp->setAttribute("completer", m_BitsAddressList);
        } else if (propName.first(1) == "n") {
            variantProp->setAttribute("completer", m_WordsAddressList);
        }
    }
}

void LocalizationTaskWidget::updateCameraMappingToWidget() {
    ui->camera_mapping_wg->setCameraList(m_localizeTask->project()->deviceManager()->cameraDevicesNameList());
    QMap<int, QString> config_map = m_config.d->m_sCameraNumberMap;
    QMap<int, QString> name_map;

    auto map_it = config_map.cbegin();
    while (map_it != config_map.cend()) {
        vc::device::IDevice *dv = m_localizeTask->project()->deviceManager()->deviceById(map_it.value()).get();
        if (dv) {
            name_map.insert(map_it.key(), dv->name());
        }
        map_it++;
    }

    ui->camera_mapping_wg->setCurrentMapping(name_map);
}

void LocalizationTaskWidget::onSelectCommDeviceClicked() {
    vc::model::Project *proj = m_localizeTask->project();

    QMap<QString, QString> devices_map = proj->deviceManager()->commDevices();
    QMap<QString, QString> ids_map;
    QStringList names;
    auto it_device = devices_map.begin();
    while (it_device != devices_map.cend()) {
        names << it_device.value();
        ids_map.insert(it_device.value(), it_device.key());
        it_device++;
    }

    bool isOk;
    QString selectedName = QInputDialog::getItem(this,
                                                 tr("Communication device"),
                                                 tr("Available devices:"),
                                                 names, 0, false, &isOk, Qt::MSWindowsFixedSizeDialogHint);

    if (isOk) {
        ui->ledit_comm_device->setText(selectedName);
        m_config.d->m_sCommDeviceId = ids_map.value(selectedName);
        updateCommDeviceInfo();
    }
}

void LocalizationTaskWidget::onUpdateCompleter() {
    m_BitsAddressList = m_commDevice->getAvailableBits();
    m_WordsAddressList = m_commDevice->getAvailableWords();
    populateBrowser();
}

void LocalizationTaskWidget::onSelectOutputDeviceClicked() {

}

void LocalizationTaskWidget::onCameraMappingChanged(const QMap<int, QString> &mapping) {
    QMap<QString, QString> camera_id = m_localizeTask->project()->deviceManager()->cameraDevices();

    QMap<int, QString> &config_map = m_config.d->m_sCameraNumberMap;
    config_map.clear();
    auto map_it = mapping.cbegin();
    while (map_it != mapping.cend()) {
        // get camera ID from name
        QString id = camera_id.key(map_it.value());

        if (!id.isEmpty()) {
            // save to map <number, ID>
            config_map.insert(map_it.key(), id);
        }
        map_it++;
    }
    m_localizeTask->setTaskLocalizeConfig(m_config);
}

void LocalizationTaskWidget::onPropertyManagerValueChanged(QtProperty *property, const QVariant &variant) {
    if (m_populating_browser) {
        return;
    }

    QString propName = property->propertyName();

    const QMetaObject *meta_task = m_localizeTask->metaObject();
    const QMetaObject &meta_config = m_config.getMetaObject();

    // abstract device
    int index = meta_task->indexOfProperty(propName.toUtf8());
    if (index != -1) {
        if (propName == "name") {
            QString new_name = variant.toString();
            // avoid loop made by signal valueChanged
            if (m_localizeTask->name() == new_name) {
                return;
            }

            // vc::model::Project *proj  = m_localizeTask->project();

            if (!m_localizeTask->project()->changeTaskName(m_localizeTask->id(), new_name)) {
                m_variantManager->setValue(property, m_localizeTask->name());
            }
        }
        return;
    }

    // mc context
    index = meta_config.indexOfProperty(propName.toUtf8());
    if (index != -1) {
        QMetaProperty mProp = meta_config.property(index);
        mProp.writeOnGadget(&m_config, variant);
        qDebug() << "Updated context: " << propName << "to" << variant;
        this->saveConfig();
        return;
    }
}

void LocalizationTaskWidget::saveConfig() {
    m_localizeTask->setTaskLocalizeConfig(m_config);
}
