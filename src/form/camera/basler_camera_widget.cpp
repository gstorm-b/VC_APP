#include "basler_camera_widget.h"
#include "ui_basler_camera_widget.h"

#include <QMessageBox>
#include <QHostAddress>

#include "windows_helper.h"

inline QPixmap cvMatToQPixmap(const cv::Mat& mat) {
    QImage qimg;
    if (mat.type() == CV_8UC1) {
        // Grayscale
        qimg = QImage(mat.data,
                      mat.cols,
                      mat.rows,
                      static_cast<int>(mat.step),
                      QImage::Format_Grayscale8);
    } else if (mat.type() == CV_8UC3) {
        // OpenCV uses BGR, Qt uses RGB
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        qimg = QImage(rgb.data,
                      rgb.cols,
                      rgb.rows,
                      static_cast<int>(rgb.step),
                      QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC4) {
        // BGRA → RGBA
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        qimg = QImage(rgba.data,
                      rgba.cols,
                      rgba.rows,
                      static_cast<int>(rgba.step),
                      QImage::Format_RGBA8888).copy();
    } else {
        return QPixmap();
    }
    return QPixmap::fromImage(qimg);
}

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
    int typeId = value.userType();
    int minIdx = meta.indexOfClassInfo(QString("%1_min").arg(propName).toUtf8());
    if (minIdx != -1) {
        if (typeId == QMetaType::Int) {
            variantProp->setAttribute("minimum", QString(meta.classInfo(minIdx).value()).toInt());
        } else if (typeId == QMetaType::Double) {
            variantProp->setAttribute("minimum", QString(meta.classInfo(minIdx).value()).toDouble());
        }
    }

    int maxIdx = meta.indexOfClassInfo(QString("%1_max").arg(propName).toUtf8());
    if (maxIdx != -1) {
        if (typeId == QMetaType::Int) {
            variantProp->setAttribute("maximum", QString(meta.classInfo(maxIdx).value()).toInt());
        } else if (typeId == QMetaType::Double) {
            variantProp->setAttribute("maximum", QString(meta.classInfo(maxIdx).value()).toDouble());
        }
    }

    if (!prop.isWritable()) {
        variantProp->setEnabled(false);
    }

    return variantProp;
}

void BaslerCameraWidget::populateBrowser_Device(vc::device::IDevice *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
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

void BaslerCameraWidget::populateBrowser_BaslerConfig(vc::device::BaslerGigeCfg *gadget, QtVariantPropertyManager *manager, QtTreePropertyBrowser *browser) {
    QtProperty *topItem = manager->addProperty(QtVariantPropertyManager::groupTypeId(),
                                               QLatin1String("Camera basler"));
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

        if (variantProp->propertyName() == "paramsExposureTime") {
            qDebug() << "exposure limit" << gadget->m_paramsExposureMin << gadget->m_paramsExposureMax;
            variantProp->setAttribute("minimum", gadget->m_paramsExposureMin);
            variantProp->setAttribute("maximum",  gadget->m_paramsExposureMax);
        } else if (variantProp->propertyName() == "paramsGain") {
            qDebug() << "gain limit" << gadget->m_paramsGainMin << gadget->m_paramsGainMax;
            variantProp->setAttribute("minimum", gadget->m_paramsGainMin);
            variantProp->setAttribute("maximum",  gadget->m_paramsGainMax);
        } else if (variantProp->propertyName() == "autoBacklightLine") {
            QStringList str_list;
            for (BaslerIOLine io : gadget->m_ioCapabilities) {
                if (io.can_be_output) {
                    str_list.append(io.name);
                }
            }
            variantProp->setAttribute("completer", str_list);
        }
    }
}

BaslerCameraWidget::BaslerCameraWidget(std::shared_ptr<vc::device::IDevice> dv,
                                       ads::CDockWidget *dock, QWidget *parent)
    : IDeviceWidget(parent),
    ui(new Ui::BaslerCameraWidget),
    m_device(dv),
    m_dock(dock) {

    ui->setupUi(this);
    initCameraWiget();
}

BaslerCameraWidget::~BaslerCameraWidget() {
    delete ui;
}

void BaslerCameraWidget::loadConfigToDevice() {
    if(!m_camera) {
        return;
    }
    m_camera->setBaslerGigeConfig(m_params);
}

void BaslerCameraWidget::loadConfigToWidget() {
    populateBrowser();
}

void BaslerCameraWidget::setDockWidget(ads::CDockWidget *dock) {
    m_dock = dock;
}

void BaslerCameraWidget::initCameraWiget() {
    ui->btn_connect->setIcon(svgIcon(":/resrc/icon/plug_disconnected.svg"));
    ui->btn_trigger->setIcon(svgIcon(":/resrc/icon/capture.svg"));

    connect(ui->btn_select_camera, &QPushButton::clicked,
            this, &BaslerCameraWidget::btn_choose_camera_clicked);
    connect(ui->btn_connect, &QPushButton::clicked,
            this, &BaslerCameraWidget::btn_connect_clicked);
    connect(ui->btn_trigger, &QPushButton::clicked,
            this, &BaslerCameraWidget::btn_trigger_clicked);
    // connect(ui->btn_auto_shot, &QPushButton::clicked,
    //         this, &BaslerCameraWidget::btn_auto_shot_clicked);

    // connect(ui->btn_baklight_toggle, &QPushButton::clicked,
    //         this, &BaslerCameraWidget::btn_backlight_toggle_clicked);
    // connect(ui->btn_save_image, &QPushButton::clicked,
    //         this, &BaslerCameraWidget::btn_save_image_clicked);

    // init property browser
    m_variantManager = new QtVariantPropertyManager(this);
    m_variantFactory = new QtVariantEditorFactory(this);
    m_variantEditor = new QtTreePropertyBrowser();

    QHBoxLayout *layout = new QHBoxLayout();
    ui->wg_property_browser->setLayout(layout);
    layout->addWidget(m_variantEditor);
    layout->setContentsMargins(0,0,0,0);

    m_variantEditor->setAlternatingRowColors(false);
    m_variantEditor->setFactoryForManager(m_variantManager, m_variantFactory);
    m_variantEditor->setPropertiesWithoutValueMarked(true);
    m_variantEditor->setRootIsDecorated(false);
    m_variantEditor->setResizeMode(QtTreePropertyBrowser::Stretch);

    if (m_device) {
        m_camera = static_cast<vc::device::BaslerGigECamera*>(m_device.get());
        connect(m_camera, &vc::device::IDevice::connectStatusChanged,
                this, &BaslerCameraWidget::onCameraConnectStatusChanged);

        m_params = m_camera->baslerGigeConfig();
        loadConfigToWidget();
        connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
                this, &BaslerCameraWidget::onPropertyValueChanged);

        // connect(m_variantManager, &QtVariantPropertyManager::valueChanged, this, [=](QtProperty *property, const QVariant &variant) {
        //     QString propName = property->propertyName();

        //     const QMetaObject *idevice_meta = m_device->metaObject();
        //     const QMetaObject &meta_cfg = m_params.getMetaObject();

        //     // abstract device
        //     int index = idevice_meta->indexOfProperty(propName.toUtf8());
        //     if (index != -1) {
        //         if (propName == "name") {
        //             QString new_name = variant.toString();
        //             // avoid loop made by signal valueChanged
        //             if (m_device->name() == new_name) {
        //                 return;
        //             }

        //             if (!m_device->deviceManager()->changeDeviceName(m_device->id(), new_name)) {
        //                 m_variantManager->setValue(property, m_device->name());
        //             }
        //         }
        //         return;
        //     }

        //     // parameters
        //     index = meta_cfg.indexOfProperty(propName.toUtf8());
        //     if (index != -1) {
        //         QMetaProperty mProp = meta_cfg.property(index);
        //         mProp.writeOnGadget(&m_params, variant);
        //         qDebug() << "Updated context: " << propName << "to" << variant;
        //         this->m_camera->setBaslerGigeConfig(m_params);
        //         if (this->m_camera->isDeviceConnected()) {
        //             this->m_worker->applyChangeParameters();
        //         }
        //         return;
        //     }
        // });

    }

    ui->splitter_main->setStretchFactor(0, 7);
    ui->splitter_main->setStretchFactor(0, 3);

    m_camera_select_dialog = new BaslerCamSelectDialog(this);

    connect(m_camera_select_dialog, &BaslerCamSelectDialog::userSelectionFinished,
            this, &BaslerCameraWidget::cameraSelectionFinished);

    m_worker = new vc::runtime::CameraWorker(m_camera);
    m_worker->moveToWorker();
    connect(m_camera, &vc::device::CameraDevice::grabFinished,
            this, &BaslerCameraWidget::onCameraGrabFinished);
}

void BaslerCameraWidget::setDoublePropertyLimit(QtVariantProperty *variant, double &max, double &min) {
    variant->setAttribute("minimum", min);
    variant->setAttribute("maximum", max);
}

void BaslerCameraWidget::setIntPropertyLimit(QtVariantProperty *variant, int &max, int &min) {
    variant->setAttribute("minimum", min);
    variant->setAttribute("maximum", max);
}

void BaslerCameraWidget::updateDockTitle() {
    if (m_dock == nullptr) {
        return;
    }
    m_dock->setWindowTitle(QString("%1").arg(m_device->name()));
}

void BaslerCameraWidget::onPropertyValueChanged(QtProperty *property, const QVariant &variant) {
    if (m_populating_browser) {
        return;
    }

    QString propName = property->propertyName();

    const QMetaObject *idevice_meta = m_device->metaObject();
    const QMetaObject &meta_cfg = m_params.getMetaObject();

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

    // parameters
    index = meta_cfg.indexOfProperty(propName.toUtf8());
    if (index != -1) {
        QMetaProperty mProp = meta_cfg.property(index);
        mProp.writeOnGadget(&m_params, variant);
        qDebug() << "Updated context: " << propName << "to" << variant;
        this->m_camera->setBaslerGigeConfig(m_params);
        if (this->m_camera->isDeviceConnected()) {
            this->m_worker->applyChangeParameters();
        }
        return;
    }
}

void BaslerCameraWidget::onCameraConnectStatusChanged(vc::device::ConnectStatus status) {
    if (status == vc::device::ConnectStatus::Connected) {
        onCameraConnected();
    } else {
        onCameraDisconnected();
    }
}

void BaslerCameraWidget::cameraSelectionFinished(bool isAccept,
                                                 Pylon::CDeviceInfo device) {
    if (isAccept) {
        m_params.m_deviceInfo = device;
        m_params.m_modelName = device.GetModelName();
        m_params.m_serialNumber = device.GetSerialNumber();
        m_params.m_userDefinedName = device.GetUserDefinedName();
        m_params.m_ipAddress = device.GetIpAddress();
        loadConfigToWidget();
        m_camera->setBaslerGigeConfig(m_params);
    }
}

void BaslerCameraWidget::btn_choose_camera_clicked() {
    m_camera_select_dialog->showCameraSelectForm();
}

void BaslerCameraWidget::btn_connect_clicked() {
    if (!m_camera->isDeviceConnected()) {
        QHostAddress address;
        if (!address.setAddress(m_params.ipAddress())) {
            QMessageBox::warning(this,
                                 tr("Connect error"),
                                 tr("IP address invalid format."));
            return;
        }

        m_worker->connectCamera();

    } else {
        m_worker->disconnectCamera();
    }
}

void BaslerCameraWidget::btn_trigger_clicked() {
    if (!m_camera->isDeviceConnected()) {
        return;
    }

    m_worker->singleShot();
}

void BaslerCameraWidget::onCameraConnected() {
    ui->lb_connection_status->setText(tr("Connected"));
    m_params = m_camera->baslerGigeConfig();
    LOG_DEV_DEBUG << m_params.m_paramsExposureMin << m_params.m_paramsExposureMax;
    LOG_DEV_DEBUG << m_params.m_paramsGainMin << m_params.m_paramsGainMax;
    loadConfigToWidget();
    ui->btn_connect->setIcon(svgIcon(":/resrc/icon/plug_connected.svg"));
}

void BaslerCameraWidget::onCameraDisconnected() {
    ui->lb_connection_status->setText(tr("Disconnected"));
    ui->btn_connect->setIcon(svgIcon(":/resrc/icon/plug_disconnected.svg"));
}

void BaslerCameraWidget::onCameraGrabFinished(vc::device::GrabResult result) {
    if (result.isGrabSuccess) {
        QPixmap new_frame = cvMatToQPixmap(result.frame);
        ui->image_view->loadImage(new_frame);
    }
}

void BaslerCameraWidget::populateBrowser() {
    m_populating_browser = true;
    m_variantEditor->blockSignals(true);
    m_variantManager->clear();

    populateBrowser_Device(m_device.get(), m_variantManager, m_variantEditor);
    populateBrowser_BaslerConfig(&m_params, m_variantManager, m_variantEditor);

    m_variantEditor->blockSignals(false);
    m_populating_browser = false    ;
}

