#include "localization_task_widget.h"
#include "ui_localization_task_widget.h"
#include "model/project.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QButtonGroup>
#include <QInputDialog>
#include <QMouseEvent>
#include <QScrollArea>
#include <QFont>

#include "windows_helper.h"
#include "form/camera/basler_camera_widget.h"
#include "form/plc/mc_protocol_device_widget.h"
#include "device/idevice_config.h"

// ──────────────────────────────────────────────────────────────────────────────
//  Device type → accent color
// ──────────────────────────────────────────────────────────────────────────────
static QColor accentForDeviceType(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return QColor(0x2b, 0x8c, 0xe8);
    case vc::device::DeviceType::McDevice: return QColor(0x22, 0xd1, 0x7a);
    case vc::device::DeviceType::Robot:    return QColor(0xf5, 0xa6, 0x23);
    default:                               return QColor(0x6b, 0x7e, 0xa0);
    }
}

static QString typeShortLabel(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return "CAM";
    case vc::device::DeviceType::McDevice: return "PLC";
    case vc::device::DeviceType::Robot:    return "BOT";
    default:                               return "DEV";
    }
}

static QString deviceIconPath(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return ":/resrc/icon/camera.svg";
    case vc::device::DeviceType::McDevice: return ":/resrc/icon/plc_icon.svg";
    case vc::device::DeviceType::Robot:    return ":/resrc/icon/robot_movement.svg";
    default:                               return ":/resrc/icon/setting.svg";
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Constructor / Destructor
// ──────────────────────────────────────────────────────────────────────────────
LocalizationTaskWidget::LocalizationTaskWidget(std::shared_ptr<vc::model::ITask> task,
                                               ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
      ui(new Ui::LocalizationTaskWidget)
{
    ui->setupUi(this);
    initWidget();
}

LocalizationTaskWidget::~LocalizationTaskWidget()
{
    delete ui;
}

void LocalizationTaskWidget::loadConfigToTask()  {}
void LocalizationTaskWidget::loadConfigToWidget() {}

// ──────────────────────────────────────────────────────────────────────────────
//  initWidget
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initWidget()
{
    if (m_task) {
        m_localizeTask = static_cast<vc::model::TaskLocalization*>(m_task.get());
        m_config       = m_localizeTask->taskLocalizeConfig();
    }

    // initPropertyBrowser(ui->wg_property_browser);
    initBrowserInWidget(ui->wg_property_browser);
    initNavPanel();
    initStatusLamps();
    initContentStack();

    ui->nav_splitter->setStretchFactor(0, 3);
    ui->nav_splitter->setStretchFactor(1, 7);

    ui->splitter_main_content->setStretchFactor(0, 7);
    ui->splitter_main_content->setStretchFactor(1, 3);

    // Settings page: comm/output device, camera mapping
    // ui->ledit_comm_device->setReadOnly(true);
    // ui->ledit_output_device->setReadOnly(true);
    // ui->ledit_comm_device->setPlaceholderText(tr("Select communication device..."));
    // ui->ledit_output_device->setPlaceholderText(tr("Select output device..."));

    // connect(ui->tbtn_select_comm_device,   &QToolButton::clicked,
    //         this, &LocalizationTaskWidget::onSelectCommDeviceClicked);
    // connect(ui->tbtn_select_output_device, &QToolButton::clicked,
    //         this, &LocalizationTaskWidget::onSelectOutputDeviceClicked);

    if (m_localizeTask) {
        // updateCameraMappingToWidget();
        // updateCommDeviceInfo();
        // updateOutputDeviceInfo();

        auto *proj = m_localizeTask->project();
        connect(proj->deviceManager().get(), &vc::device::DeviceManager::devicesChanged,
                this, [this]() {
                  // ui->camera_mapping_wg->setCameraList(
                  //     m_localizeTask->project()->deviceManager()->cameraDevicesNameList());
                  // rebuildDeviceNav();
                  onTaskDevicesChanged();
        });
        // connect(ui->camera_mapping_wg, &CameraMappingWidget::mappingChanged,
        //         this, &LocalizationTaskWidget::onCameraMappingChanged);

        connect(m_localizeTask, &vc::model::ITask::devicesChanged,
                this, &LocalizationTaskWidget::onTaskDevicesChanged);
    }

    // connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
    //         this, &LocalizationTaskWidget::onPropertyManagerValueChanged);

    // Toolbar buttons in breadcrumb
    connect(ui->tbtn_bc_save,    &QToolButton::clicked,
            this, &LocalizationTaskWidget::saveConfig);
    // connect(ui->tbtn_bc_refresh, &QToolButton::clicked,
    //         this, [this]() { populateBrowser(); });

    // Add device button
    connect(ui->tbtn_add_device, &QToolButton::clicked, this, [this]() {
        emit addDeviceRequested(m_task ? m_task->id() : QString());
    });

    // Dashboard tab (default active)
    showDashboardPage();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Nav panel init
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initNavPanel()
{
    // Task header labels
    QFont sectionFont = ui->lbl_active_title->font();
    sectionFont.setPointSizeF(7.5);
    sectionFont.setBold(true);
    sectionFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    ui->lbl_active_title->setFont(sectionFont);
    ui->lbl_active_title->setText(tr("ACTIVE TASK"));

    QFont nameFont;
    nameFont.setFamily("JetBrains Mono");
    if (!nameFont.exactMatch()) nameFont.setFamily("Consolas");
    nameFont.setPointSizeF(10);
    nameFont.setBold(true);
    ui->lbl_task_name_nav->setFont(nameFont);
    ui->lbl_task_name_nav->setWordWrap(true);
    ui->lbl_task_name_nav->setText(m_task ? m_task->name() : tr("Task"));

    QFont chipFont = ui->lbl_task_type_chip->font();
    chipFont.setPointSizeF(8);
    chipFont.setBold(true);
    chipFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
    ui->lbl_task_type_chip->setFont(chipFont);
    ui->lbl_task_type_chip->setText(tr("LOC"));

    // Devices section label
    QFont devSecFont = ui->lbl_devices_section->font();
    devSecFont.setPointSizeF(7.5);
    devSecFont.setBold(true);
    devSecFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.8);
    ui->lbl_devices_section->setFont(devSecFont);
    ui->lbl_devices_section->setText(tr("DEVICES"));

    // Add device button
    ui->tbtn_add_device->setIcon(svgIcon(":/resrc/icon/plus_square.svg", 14));
    ui->tbtn_add_device->setToolTip(tr("Add device to task"));

    // Nav buttons
    ui->btn_nav_dashboard->setText(tr("  Dashboard"));
    ui->btn_nav_dashboard->setIcon(svgIcon(":/resrc/icon/dashboard.svg", 14));
    ui->btn_nav_dashboard->setIconSize({14, 14});

    ui->btn_nav_settings->setText(tr("  Settings"));
    ui->btn_nav_settings->setIcon(svgIcon(":/resrc/icon/setting.svg", 14));
    ui->btn_nav_settings->setIconSize({14, 14});

    m_navBtnGroup = new QButtonGroup(this);
    m_navBtnGroup->addButton(ui->btn_nav_dashboard);
    m_navBtnGroup->addButton(ui->btn_nav_settings);
    m_navBtnGroup->setExclusive(true);

    connect(ui->btn_nav_dashboard, &QPushButton::clicked,
            this, &LocalizationTaskWidget::showDashboardPage);
    connect(ui->btn_nav_settings,  &QPushButton::clicked,
            this, &LocalizationTaskWidget::showSettingsPage);

    // Device scroll content widget
    m_devListWidget = new QWidget(ui->scroll_devices_content);
    auto *devLayout = new QVBoxLayout(m_devListWidget);
    devLayout->setContentsMargins(8, 0, 8, 4);
    devLayout->setSpacing(1);

    auto *outerLayout = new QVBoxLayout(ui->scroll_devices_content);
    outerLayout->setContentsMargins(0, 0, 0, 0);
    outerLayout->setSpacing(0);
    outerLayout->addWidget(m_devListWidget);

    // Breadcrumb labels
    QFont bcMonoFont;
    bcMonoFont.setFamily("JetBrains Mono");
    if (!bcMonoFont.exactMatch()) bcMonoFont.setFamily("Consolas");

    bcMonoFont.setPointSizeF(8.5);
    ui->lbl_bc_task->setFont(bcMonoFont);

    QFont bcSepFont = ui->lbl_bc_sep->font();
    bcSepFont.setPointSizeF(13);
    ui->lbl_bc_sep->setFont(bcSepFont);

    bcMonoFont.setPointSizeF(9.5);
    bcMonoFont.setBold(true);
    ui->lbl_bc_current->setFont(bcMonoFont);

    ui->lbl_bc_task->setText(m_task ? m_task->name() : QString());

    // Breadcrumb toolbar buttons
    ui->tbtn_bc_save->setIcon(svgIcon(":/resrc/icon/task_save.svg", 14));
    ui->tbtn_bc_save->setIconSize({14, 14});
    ui->tbtn_bc_save->setToolTip(tr("Save"));

    ui->tbtn_bc_refresh->setIcon(svgIcon(":/resrc/icon/reload.svg", 14));
    ui->tbtn_bc_refresh->setIconSize({14, 14});
    ui->tbtn_bc_refresh->setToolTip(tr("Refresh"));

    rebuildDeviceNav();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Status lamps
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initStatusLamps()
{
    const QStringList labels = { "READY", "CAM", "PLC", "BOT" };

    auto *hl = qobject_cast<QHBoxLayout*>(ui->frame_status_lamps->layout());
    if (!hl) return;

    hl->addStretch();
    for (const auto &label : labels) {
        // Vertical pair: dot + text
        auto *col     = new QWidget(ui->frame_status_lamps);
        auto *colLay  = new QVBoxLayout(col);
        colLay->setContentsMargins(0, 0, 0, 0);
        colLay->setSpacing(3);
        colLay->setAlignment(Qt::AlignHCenter);

        auto *dot = new QFrame(col);
        dot->setFixedSize(10, 10);

        QFont lf;
        lf.setPointSizeF(6.5);
        lf.setBold(true);
        lf.setLetterSpacing(QFont::AbsoluteSpacing, 0.5);
        auto *lbl = new QLabel(label, col);
        lbl->setFont(lf);
        lbl->setAlignment(Qt::AlignCenter);

        colLay->addWidget(dot, 0, Qt::AlignHCenter);
        colLay->addWidget(lbl, 0, Qt::AlignHCenter);

        hl->addWidget(col);
        m_statusLamps.append({dot, lbl});
    }
    hl->addStretch();

    updateStatusLamps();
}

void LocalizationTaskWidget::updateStatusLamps()
{
    // States: [0]=READY, [1]=CAM, [2]=PLC, [3]=BOT
    bool ready = false, hasCam = false, hasPlc = false, hasBot = false;

    if (m_localizeTask && m_localizeTask->project()) {
        auto *mgr = m_localizeTask->project()->deviceManager().get();
        for (const QString &id : m_localizeTask->assignedDeviceIds()) {
            auto dev = mgr->deviceById(id);
            if (!dev) continue;
            switch (dev->deviceType()) {
            case vc::device::DeviceType::Camera:   hasCam = true; break;
            case vc::device::DeviceType::McDevice: hasPlc = true; break;
            case vc::device::DeviceType::Robot:    hasBot = true; break;
            default: break;
            }
        }
        ready = hasCam || hasPlc || hasBot;
    }

    const bool states[4] = { ready, hasCam, hasPlc, hasBot };
    for (int i = 0; i < m_statusLamps.size(); ++i) {
        bool on = (i < 4) ? states[i] : false;
        QColor col  = on ? QColor(0x22, 0xd1, 0x7a) : QColor(0xe8, 0x40, 0x40);
        QString css = QString(
            "QFrame {"
            "  border-radius: 5px;"
            "  background: qradialgradient(cx:0.35,cy:0.35,radius:0.65,"
            "    stop:0 %1, stop:1 %2);"
            "  border: 1px solid %3;"
            "}")
            .arg(on ? "#2eff98" : "#ff5b5b",
                 col.name(),
                 col.name() + "66");
        m_statusLamps[i].dot->setStyleSheet(css);
        m_statusLamps[i].label->setStyleSheet(
            QString("color: %1;").arg(on ? "#2a3a52" : "#3a2a2a"));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Content stack init
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initContentStack()
{
    // Insert a dashboard placeholder at index 0
    // page_task_config (from .ui) shifts to index kSettingsPage = 1
    m_dashboardPage = new QWidget(this);
    auto *dashLayout = new QVBoxLayout(m_dashboardPage);
    dashLayout->setAlignment(Qt::AlignCenter);
    auto *dashLbl = new QLabel(tr("Dashboard"), m_dashboardPage);
    QFont f = dashLbl->font();
    f.setPointSize(12);
    dashLbl->setFont(f);
    dashLbl->setAlignment(Qt::AlignCenter);
    dashLbl->setStyleSheet("color: #3a4f6a;");
    dashLayout->addWidget(dashLbl);

    ui->content_stack->insertWidget(kDashboardPage, m_dashboardPage);
    // Now: 0=dashboard, 1=page_task_config (settings)
}

void LocalizationTaskWidget::onTaskDevicesChanged() {
    QStringList deviceIds = m_localizeTask->assignedDeviceIds();
    for (int idx=0;idx<ui->content_stack->count();idx++) {
        QWidget *w = ui->content_stack->widget(idx);
        IDeviceWidget *dw = qobject_cast<IDeviceWidget*>(w);
        if (dw) {
            const QString deviceId = dw->deviceId();
            if (!deviceIds.contains(deviceId)) {
                ui->content_stack->removeWidget(w);
                w->deleteLater();
            }
            removePropertyBrowserWidget(dw->getPropertyBrowser());
            m_devicePages.remove(deviceId);
        }
    }

    rebuildDeviceNav();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Device nav rebuild
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::rebuildDeviceNav()
{
    // Clear existing items
    QLayout *lay = m_devListWidget->layout();
    while (QLayoutItem *it = lay->takeAt(0)) {
        if (QWidget *w = it->widget()) w->deleteLater();
        delete it;
    }
    m_navItems.clear();

    if (!m_localizeTask || !m_localizeTask->project()) return;

    auto *mgr = m_localizeTask->project()->deviceManager().get();
    bool anyDevice = false;

    for (const QString &devId : m_localizeTask->assignedDeviceIds()) {
        auto device = mgr->deviceById(devId);
        if (!device) continue;
        anyDevice = true;

        QColor accent = accentForDeviceType(device->deviceType());

        auto *item = new QFrame(m_devListWidget);
        item->setAttribute(Qt::WA_Hover);
        item->setCursor(Qt::PointingHandCursor);
        item->installEventFilter(this);

        auto *itemLayout = new QHBoxLayout(item);
        itemLayout->setContentsMargins(7, 7, 7, 7);
        itemLayout->setSpacing(7);

        // Icon + status dot container
        auto *iconBox = new QWidget(item);
        iconBox->setFixedSize(20, 20);
        iconBox->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *iconLbl = new QLabel(iconBox);
        iconLbl->setPixmap(svgIcon(deviceIconPath(device->deviceType())).pixmap(14, 14));
        iconLbl->move(0, 3);
        iconLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

        // Status dot
        auto *dotLbl = new QFrame(iconBox);
        dotLbl->setFixedSize(5, 5);
        dotLbl->move(13, 13);
        dotLbl->setStyleSheet(
            "QFrame { border-radius: 2px; background: #e84040; border: 1px solid #09111e; }");
        dotLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

        itemLayout->addWidget(iconBox);

        // Name + type label
        auto *textCol = new QWidget(item);
        textCol->setAttribute(Qt::WA_TransparentForMouseEvents);
        auto *textLay = new QVBoxLayout(textCol);
        textLay->setContentsMargins(0, 0, 0, 0);
        textLay->setSpacing(1);

        auto *nameLbl = new QLabel(device->name(), textCol);
        QFont nameFont("JetBrains Mono");
        if (!nameFont.exactMatch()) nameFont.setFamily("Consolas");
        nameFont.setPointSizeF(9);
        nameLbl->setFont(nameFont);
        nameLbl->setObjectName("devNavName");
        nameLbl->setStyleSheet("color: #6b7ea0;");
        nameLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

        auto *typeLbl = new QLabel(typeShortLabel(device->deviceType()), textCol);
        QFont typeFont;
        typeFont.setPointSizeF(7.5);
        typeFont.setBold(true);
        typeFont.setLetterSpacing(QFont::AbsoluteSpacing, 0.4);
        typeLbl->setFont(typeFont);
        typeLbl->setStyleSheet(QString("color: %1;").arg(accent.name()));
        typeLbl->setAttribute(Qt::WA_TransparentForMouseEvents);

        textLay->addWidget(nameLbl);
        textLay->addWidget(typeLbl);
        itemLayout->addWidget(textCol, 1);

        m_navItems.insert(devId, item);
        lay->addWidget(item);
    }

    // Empty state
    if (!anyDevice) {
        auto *emptyLbl = new QLabel(tr("No devices assigned.\nUse [+] to add."),
                                    m_devListWidget);
        emptyLbl->setAlignment(Qt::AlignCenter);
        QFont f = emptyLbl->font();
        f.setPointSizeF(8.5);
        emptyLbl->setFont(f);
        emptyLbl->setWordWrap(true);
        emptyLbl->setStyleSheet("color: #1a2a3a;");
        emptyLbl->setContentsMargins(4, 8, 4, 8);
        lay->addWidget(emptyLbl);
    }

    static_cast<QVBoxLayout*>(lay)->addStretch();
    refreshNavItemStyles();
    updateStatusLamps();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Navigation
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::showDashboardPage()
{
    m_activeDeviceId.clear();
    ui->content_stack->setCurrentIndex(kDashboardPage);
    ui->btn_nav_dashboard->setChecked(true);
    refreshNavItemStyles();
    updateBreadcrumb(tr("Dashboard"), QColor(0x2b, 0x8c, 0xe8));
    // populateBrowser();
    ui->wg_property_browser->setVisible(false);
}

void LocalizationTaskWidget::showSettingsPage()
{
    m_activeDeviceId.clear();
    ui->content_stack->setCurrentIndex(kSettingsPage);
    ui->btn_nav_settings->setChecked(true);
    refreshNavItemStyles();
    updateBreadcrumb(tr("Settings"), QColor(0x6b, 0x7e, 0xa0));
    // populateBrowser();
    ui->wg_property_browser->setVisible(false);
}

void LocalizationTaskWidget::showDeviceConfigPage(const QString &deviceId)
{
    QWidget *page = getOrCreateDeviceConfigPage(deviceId);
    if (!page) return;

    m_activeDeviceId = deviceId;
    ui->content_stack->setCurrentWidget(page);

    // Uncheck nav buttons
    ui->btn_nav_dashboard->setChecked(false);
    ui->btn_nav_settings->setChecked(false);

    refreshNavItemStyles();

    // Update breadcrumb
    QColor accent = colorForDeviceId(deviceId);
    QString label;
    if (m_localizeTask && m_localizeTask->project()) {
        auto dev = m_localizeTask->project()->deviceById(deviceId);
        if (dev) label = dev->name();
    }
    updateBreadcrumb(label.isEmpty() ? deviceId : label, accent);
    populateBrowser(deviceId);

    ui->wg_property_browser->setVisible(true);
}

void LocalizationTaskWidget::onDeviceNavClicked(const QString &deviceId)
{
    showDeviceConfigPage(deviceId);
}

void LocalizationTaskWidget::refreshNavItemStyles()
{
    for (auto it = m_navItems.cbegin(); it != m_navItems.cend(); ++it) {
        QFrame *item   = it.value();
        bool    active = (it.key() == m_activeDeviceId);
        QColor  accent = colorForDeviceId(it.key());

        if (active) {
            item->setStyleSheet(QString(
                "QFrame {"
                "  border: 1px solid %1;"
                "  background-color: rgba(%2, %3, %4, 21);"
                "  border-radius: 5px;"
                "}")
                .arg(accent.name() + "66")
                .arg(accent.red()).arg(accent.green()).arg(accent.blue()));

            if (auto *lbl = item->findChild<QLabel*>("devNavName"))
                lbl->setStyleSheet("color: #dce8f5; font-weight: bold;");
        } else {
            item->setStyleSheet(
                "QFrame {"
                "  border: 1px solid transparent;"
                "  background: transparent;"
                "  border-radius: 5px;"
                "}"
                "QFrame:hover { background: #0f1a2a; }");
            if (auto *lbl = item->findChild<QLabel*>("devNavName"))
                lbl->setStyleSheet("color: #6b7ea0;");
        }
    }
}

void LocalizationTaskWidget::updateBreadcrumb(const QString &label, const QColor &color)
{
    ui->lbl_bc_current->setText(label);
    ui->lbl_bc_current->setStyleSheet(QString("color: %1;").arg(color.name()));
}

// ──────────────────────────────────────────────────────────────────────────────
//  Helpers
// ──────────────────────────────────────────────────────────────────────────────
QWidget *LocalizationTaskWidget::getOrCreateDeviceConfigPage(const QString &deviceId)
{
    if (m_devicePages.contains(deviceId))
        return m_devicePages.value(deviceId);

    if (!m_localizeTask || !m_localizeTask->project()) return nullptr;

    auto device = m_localizeTask->project()->deviceById(deviceId);
    if (!device) return nullptr;

    // device widget factory

    QWidget *page = nullptr;
    switch (device->deviceType()) {
    case vc::device::DeviceType::Camera:
        page = new BaslerCameraWidget(device, nullptr, this);
        break;
    case vc::device::DeviceType::McDevice:
        page = new McProtocolDeviceWidget(device, nullptr, this);
        break;
    default: {
        page = new QWidget(this);
        auto *l   = new QVBoxLayout(page);
        auto *lbl = new QLabel(tr("No configuration panel available for this device."), page);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet("color: #3a4f6a; font-size: 11pt;");
        l->addWidget(lbl);
        break;
    }
    }

    ui->content_stack->addWidget(page);
    m_devicePages.insert(deviceId, page);
    return page;
}

QColor LocalizationTaskWidget::colorForDeviceId(const QString &deviceId) const
{
    if (!m_localizeTask || !m_localizeTask->project()) return QColor(0x6b, 0x7e, 0xa0);
    auto dev = m_localizeTask->project()->deviceById(deviceId);
    if (!dev) return QColor(0x6b, 0x7e, 0xa0);
    return accentForDeviceType(dev->deviceType());
}

// ──────────────────────────────────────────────────────────────────────────────
//  Event filter — device nav item clicks + hover
// ──────────────────────────────────────────────────────────────────────────────
bool LocalizationTaskWidget::eventFilter(QObject *obj, QEvent *ev)
{
    const QEvent::Type t = ev->type();

    if (t == QEvent::MouseButtonPress) {
        for (auto it = m_navItems.cbegin(); it != m_navItems.cend(); ++it) {
            if (obj == it.value()) {
                onDeviceNavClicked(it.key());
                return true;
            }
        }
    }

    return ITaskWidget::eventFilter(obj, ev);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Property browser population
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::populateBrowser(const QString &id)
{
    // m_variantEditor->blockSignals(true);
    // m_variantManager->clear();
    m_populating_browser = true;

    QWidget *w = m_devicePages.value(id, nullptr);
    if (w) {
        IDeviceWidget *dw = qobject_cast<IDeviceWidget*>(w);
        if (dw) {
            changePropertyBrowserWidget(dw->getPropertyBrowser());
        }
    }

    // if (m_devicePages.contains(id)) {
    //     m_devicePages.value(id)->doSetProperty()


    // }
    // m_propBrowser

    // populateBrowser_Task();
    // populateBrowser_Config();
    m_populating_browser = false;
    // m_variantEditor->blockSignals(false);
}

// void LocalizationTaskWidget::populateBrowser_Task()
// {
//     auto *topItem = m_variantManager->addProperty(
//         QtVariantPropertyManager::groupTypeId(), tr("Task information"));
//     m_variantEditor->addProperty(topItem);
//     const QMetaObject *meta = m_localizeTask->metaObject();
//     int count = meta->propertyCount();
//     for (int i = 0; i < count; ++i) {
//         QMetaProperty mp = meta->property(i);
//         QVariant val     = m_localizeTask->property(mp.name());
//         auto *vp = LocalizationTaskWidget::addPropertyToBrowser(
//             *meta, mp, val, m_variantManager, m_variantEditor);
//         if (vp) topItem->addSubProperty(vp);
//     }
// }

// void LocalizationTaskWidget::populateBrowser_Config()
// {
//     auto *topItem = m_variantManager->addProperty(
//         QtVariantPropertyManager::groupTypeId(), tr("Configuration"));
//     m_variantEditor->addProperty(topItem);
//     const QMetaObject &meta = m_config.getMetaObject();
//     int count = meta.propertyCount();
//     for (int i = 0; i < count; ++i) {
//         QMetaProperty mp = meta.property(i);
//         QVariant val     = mp.readOnGadget(&m_config);
//         auto *vp = LocalizationTaskWidget::addPropertyToBrowser(
//             meta, mp, val, m_variantManager, m_variantEditor);
//         if (!vp) continue;
//         topItem->addSubProperty(vp);
//         QString pn = mp.name();
//         if (!pn.isEmpty()) {
//             if (pn.front() == 'b') vp->setAttribute("completer", m_BitsAddressList);
//             else if (pn.front() == 'n') vp->setAttribute("completer", m_WordsAddressList);
//         }
//     }
// }

// ──────────────────────────────────────────────────────────────────────────────
//  Settings page: device info updates
// ──────────────────────────────────────────────────────────────────────────────
// void LocalizationTaskWidget::updateCommDeviceInfo()
// {
//     if (!m_localizeTask) return;
//     if (m_commDevice) {
//         disconnect(m_commDevice.get(), &vc::device::IDevice::configChanged,
//                    this, &LocalizationTaskWidget::onUpdateCompleter);
//     }
//     m_commDevice = m_localizeTask->project()->deviceById(m_config.d->m_sCommDeviceId);
//     if (m_commDevice) {
//         ui->ledit_comm_device->setText(m_commDevice->name());
//         connect(m_commDevice.get(), &vc::device::IDevice::configChanged,
//                 this, &LocalizationTaskWidget::onUpdateCompleter);
//         onUpdateCompleter();
//     } else {
//         populateBrowser();
//     }
// }

// void LocalizationTaskWidget::updateOutputDeviceInfo()
// {
//     if (!m_localizeTask) return;
//     auto dev = m_localizeTask->project()->deviceById(m_config.d->m_sOutputDeviceId);
//     if (dev) ui->ledit_output_device->setText(dev->name());
// }

// void LocalizationTaskWidget::updateCameraMappingToWidget()
// {
//     ui->camera_mapping_wg->setCameraList(
//         m_localizeTask->project()->deviceManager()->cameraDevicesNameList());

//     QMap<int, QString> config_map = m_config.d->m_sCameraNumberMap;
//     QMap<int, QString> name_map;
//     for (auto it = config_map.cbegin(); it != config_map.cend(); ++it) {
//         auto *dv = m_localizeTask->project()->deviceManager()->deviceById(it.value()).get();
//         if (dv) name_map.insert(it.key(), dv->name());
//     }
//     ui->camera_mapping_wg->setCurrentMapping(name_map);
// }

// // ──────────────────────────────────────────────────────────────────────────────
// //  Slots
// // ──────────────────────────────────────────────────────────────────────────────
// void LocalizationTaskWidget::onSelectCommDeviceClicked()
// {
//     if (!m_localizeTask) return;
//     auto *proj = m_localizeTask->project();
//     QMap<QString, QString> devMap  = proj->deviceManager()->commDevices();
//     QMap<QString, QString> idsMap;
//     QStringList names;
//     for (auto it = devMap.cbegin(); it != devMap.cend(); ++it) {
//         names << it.value();
//         idsMap.insert(it.value(), it.key());
//     }
//     bool ok = false;
//     QString sel = QInputDialog::getItem(this, tr("Communication device"),
//                                         tr("Available devices:"), names, 0, false, &ok,
//                                         Qt::MSWindowsFixedSizeDialogHint);
//     if (ok) {
//         ui->ledit_comm_device->setText(sel);
//         m_config.d->m_sCommDeviceId = idsMap.value(sel);
//         updateCommDeviceInfo();
//     }
// }

// void LocalizationTaskWidget::onSelectOutputDeviceClicked() {}

// void LocalizationTaskWidget::onUpdateCompleter()
// {
//     m_BitsAddressList  = m_commDevice->getAvailableBits();
//     m_WordsAddressList = m_commDevice->getAvailableWords();
//     populateBrowser();
// }

// void LocalizationTaskWidget::onCameraMappingChanged(const QMap<int, QString> &mapping)
// {
//     auto cameraIds = m_localizeTask->project()->deviceManager()->cameraDevices();
//     auto &config_map = m_config.d->m_sCameraNumberMap;
//     config_map.clear();
//     for (auto it = mapping.cbegin(); it != mapping.cend(); ++it) {
//         QString id = cameraIds.key(it.value());
//         if (!id.isEmpty()) config_map.insert(it.key(), id);
//     }
//     m_localizeTask->setTaskLocalizeConfig(m_config);
// }

// void LocalizationTaskWidget::onPropertyManagerValueChanged(QtProperty *property,
//                                                             const QVariant &variant)
// {
//     if (m_populating_browser) return;

//     const QString propName = property->propertyName();
//     const QMetaObject *meta_task   = m_localizeTask->metaObject();
//     const QMetaObject &meta_config = m_config.getMetaObject();

//     int idx = meta_task->indexOfProperty(propName.toUtf8());
//     if (idx != -1) {
//         if (propName == "name") {
//             if (m_localizeTask->name() == variant.toString()) return;
//             if (!m_localizeTask->project()->changeTaskName(m_localizeTask->id(),
//                                                            variant.toString())) {
//                 m_variantManager->setValue(property, m_localizeTask->name());
//             }
//         }
//         return;
//     }

//     idx = meta_config.indexOfProperty(propName.toUtf8());
//     if (idx != -1) {
//         QMetaProperty mp = meta_config.property(idx);
//         mp.writeOnGadget(&m_config, variant);
//         saveConfig();
//     }
// }

void LocalizationTaskWidget::saveConfig()
{
    if (m_localizeTask) m_localizeTask->setTaskLocalizeConfig(m_config);
}
