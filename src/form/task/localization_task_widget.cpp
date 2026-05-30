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
#include "form/vision_output/vision_output_device_widget.h"
#include "device/idevice_config.h"
#include "runtime/camera_runner.h"
#include "runtime/plc_runner.h"
#include "runtime/vision_output_runner.h"
#include "form/pattern/pattern_theme.h"
#include "form/task/localization_dashboard_widget.h"
#include "form/task/localization_patterns_widget.h"
#include "form/task/localization_setting_widget.h"

// ──────────────────────────────────────────────────────────────────────────────
//  Device type → accent color
// ──────────────────────────────────────────────────────────────────────────────
static QColor accentForDeviceType(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return QColor(0x2b, 0x8c, 0xe8);
    case vc::device::DeviceType::PLC: return QColor(0x22, 0xd1, 0x7a);
    case vc::device::DeviceType::Robot:    return QColor(0xf5, 0xa6, 0x23);
    default:                               return QColor(0x6b, 0x7e, 0xa0);
    }
}

static QString typeShortLabel(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return "CAM";
    case vc::device::DeviceType::PLC: return "PLC";
    case vc::device::DeviceType::Robot:    return "BOT";
    default:                               return "DEV";
    }
}

static QString deviceIconPath(vc::device::DeviceType t)
{
    switch (t) {
    case vc::device::DeviceType::Camera:   return ":/resrc/icon/camera.svg";
    case vc::device::DeviceType::PLC: return ":/resrc/icon/plc_icon.svg";
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
    // Stop all per-device threads before tearing down the widget.  This
    // moves devices back to the main thread so DeviceManager can keep
    // using them.  The TaskRunner stays alive (owned by ITask) so other
    // task widgets / future runs can re-enter Commission cleanly.
    if (m_localizeTask) m_localizeTask->endCommission();

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

    // ── Apply design-handoff dark theme ─────────────────────────────────────
    // Tokens come from `form/pattern/pattern_theme.h`, which mirrors the
    // design handoff `ui_scratch/design_handoff_full_project/README.md`.
    //
    // The .ui file has no styling — every visual is set programmatically.
    // We install one global stylesheet that themes the major frames, then
    // helper methods (rebuildDeviceNav, updateStatusLamps, refreshNavItemStyles)
    // override per-item colors using the same tokens.
    setStyleSheet(QString(
        // ── root ────────────────────────────────────────────────────────────
        "QWidget#LocalizationTaskWidget { background: %1; color: %2; "
        "  font-family: 'Segoe UI'; }"
        "QSplitter::handle { background: %3; }"
        "QScrollBar:vertical { background: %1; width: 8px; }"
        "QScrollBar::handle:vertical { background: %4; border-radius: 3px; min-height: 20px; }"
        "QScrollBar::handle:vertical:hover { background: %5; }"
        "QScrollBar::add-line, QScrollBar::sub-line { height: 0; background: transparent; }"

        // ── Left nav panel ──────────────────────────────────────────────────
        "QFrame#nav_panel { background: %1; border-right: 1px solid #2a2d2e; }"
        "QFrame#frame_task_header { background: %1; "
        "  border-bottom: 1px solid #2a2d2e; }"
        "QFrame#frame_status_lamps { background: %1; "
        "  border-bottom: 1px solid #2a2d2e; }"
        "QFrame#frame_nav_top { background: %1; }"
        "QFrame#frame_devices_header { background: %1; }"
        "QScrollArea#scroll_devices { background: %1; border: none; }"
        "QWidget#scroll_devices_content { background: %1; }"

        // ── Task header labels ──────────────────────────────────────────────
        "QLabel#lbl_active_title {"
        "  color: %6; font: 700 7.5pt 'Segoe UI'; letter-spacing: 1.6px; }"
        "QLabel#lbl_task_name_nav {"
        "  color: %2; font: 700 11pt 'JetBrains Mono'; }"
        "QLabel#lbl_task_type_chip {"
        "  color: %7; background: rgba(43,140,232,30); border-radius: 3px;"
        "  padding: 1px 6px; font: 700 8pt 'Segoe UI'; letter-spacing: 1px; }"
        "QLabel#lbl_devices_section {"
        "  color: %6; font: 700 7.5pt 'Segoe UI'; letter-spacing: 1.6px; }"

        // ── Nav buttons (Dashboard / Patterns / Settings) ───────────────────
        "QPushButton#btn_nav_dashboard, QPushButton#btn_nav_patterns,"
        " QPushButton#btn_nav_settings {"
        "  text-align: left; padding: 7px 12px;"
        "  background: transparent; border: 1px solid transparent; border-left: 2px solid transparent;"
        "  color: %4; font: 600 10.5pt 'Segoe UI'; border-radius: 3px; }"
        "QPushButton#btn_nav_dashboard:hover, QPushButton#btn_nav_patterns:hover,"
        " QPushButton#btn_nav_settings:hover {"
        "  background: %8; color: %4; }"
        "QPushButton#btn_nav_dashboard:checked, QPushButton#btn_nav_patterns:checked,"
        " QPushButton#btn_nav_settings:checked {"
        "  background: rgba(43,140,232,28); color: %7;"
        "  border-left: 2px solid %7; }"

        // ── Top-right breadcrumb ───────────────────────────────────────────
        "QFrame#frame_breadcrumb { background: %9; "
        "  border-bottom: 1px solid %3; min-height: 36px; max-height: 36px; }"
        "QLabel#lbl_bc_task { color: %4; font: 8.5pt 'JetBrains Mono'; }"
        "QLabel#lbl_bc_sep  { color: %6; font: 13pt 'JetBrains Mono'; }"
        "QLabel#lbl_bc_current { color: %7; font: 700 9.5pt 'JetBrains Mono'; }"

        // ── Breadcrumb tool buttons ─────────────────────────────────────────
        "QToolButton#tbtn_bc_save, QToolButton#tbtn_bc_refresh,"
        "QToolButton#tbtn_add_device {"
        "  background: transparent; border: 1px solid transparent; border-radius: 3px;"
        "  padding: 3px 6px; color: %4; }"
        "QToolButton#tbtn_bc_save:hover, QToolButton#tbtn_bc_refresh:hover,"
        "QToolButton#tbtn_add_device:hover {"
        "  background: %8; border: 1px solid %3; color: %2; }"

        // ── Right content (stack + property browser) ───────────────────────
        "QStackedWidget#content_stack { background: %1; }"
        "QWidget#wg_property_browser { background: %1; }"
        "QWidget#page_task_config   { background: %1; }"
    ).arg(ptn::BG,    /* %1 */ ptn::TXT,  /* %2 */ ptn::BD,    /* %3 */
           ptn::TXT2, /* %4 */ ptn::BD2,  /* %5 */ ptn::TXT3,  /* %6 */
           ptn::ACC,  /* %7 */ ptn::SURF2,/* %8 */ ptn::SURF   /* %9 */));

    // initPropertyBrowser(ui->wg_property_browser);
    initBrowserInWidget(ui->wg_property_browser);
    initNavPanel();
    initStatusLamps();
    // initContentStack();

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

    // ── Enter Commission phase ────────────────────────────────────────────
    // Each assigned device gets its own HighPriority QThread so the GUI
    // never blocks while we configure / connect / single-shot.  When the
    // user adds a device later, devicesChanged → syncRunnersWithDevices()
    // registers it; TaskRunner auto-starts the new runner because we are
    // already past Idle.
    if (m_localizeTask) {
        m_localizeTask->beginCommission();
    }

    ui->nav_splitter->setStretchFactor(0, 3);
    ui->nav_splitter->setStretchFactor(1, 7);

    ui->splitter_main_content->setStretchFactor(0, 7);
    ui->splitter_main_content->setStretchFactor(1, 3);

    // Dashboard tab (default active)
    showDashboardPage();
}

// ──────────────────────────────────────────────────────────────────────────────
//  Nav panel init
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initNavPanel()
{
    // ── Per design handoff: nav panel is 188px wide ─────────────────────────
    // README.md → "Width 248px sidebar; left device nav 188px".
    if (ui->nav_panel) {
        ui->nav_panel->setMinimumWidth(188);
        ui->nav_panel->setMaximumWidth(260);
    }

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

    // Nav buttons — order: Dashboard → Patterns → Settings.  All three
    // live in the .ui (vl_nav_top) so they share the same stylesheet
    // selectors and parent layout.
    ui->btn_nav_dashboard->setText(tr("  Dashboard"));
    ui->btn_nav_dashboard->setIcon(svgIcon(":/resrc/icon/dashboard.svg", 14));
    ui->btn_nav_dashboard->setIconSize({14, 14});

    ui->btn_nav_patterns->setText(tr("  Patterns"));
    ui->btn_nav_patterns->setIcon(svgIcon(":/resrc/icon/setting.svg", 14));
    ui->btn_nav_patterns->setIconSize({14, 14});

    ui->btn_nav_settings->setText(tr("  Settings"));
    ui->btn_nav_settings->setIcon(svgIcon(":/resrc/icon/setting.svg", 14));
    ui->btn_nav_settings->setIconSize({14, 14});

    m_navBtnGroup = new QButtonGroup(this);
    m_navBtnGroup->addButton(ui->btn_nav_dashboard);
    m_navBtnGroup->addButton(ui->btn_nav_patterns);
    m_navBtnGroup->addButton(ui->btn_nav_settings);
    m_navBtnGroup->setExclusive(true);

    connect(ui->btn_nav_dashboard, &QPushButton::clicked,
            this, &LocalizationTaskWidget::showDashboardPage);
    connect(ui->btn_nav_patterns,  &QPushButton::clicked,
            this, &LocalizationTaskWidget::showPatternsPage);
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
    const QStringList labels = { "READY", "CAM", "PLC", "OUT" };

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
            case vc::device::DeviceType::PLC: hasPlc = true; break;
            case vc::device::DeviceType::Robot:    hasBot = true; break;
            default: break;
            }
        }
        ready = hasCam || hasPlc || hasBot;
    }

    const bool states[4] = { ready, hasCam, hasPlc, hasBot };
    for (int i = 0; i < m_statusLamps.size(); ++i) {
        bool on = (i < 4) ? states[i] : false;
        // Design-handoff lamp: radial gradient between bright stop and base.
        // ON  → ptn::OK  (#22d17a) glow
        // OFF → ptn::ERR (#e84040) glow
        QColor col(on ? ptn::OK : ptn::ERR);
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
        // Label color from design tokens — muted for off, regular text for on.
        m_statusLamps[i].label->setStyleSheet(QString(
            "color: %1; font: 700 6.5pt 'Segoe UI'; letter-spacing: 0.5px;"
        ).arg(on ? ptn::TXT2 : ptn::TXT4));
    }
}

// ──────────────────────────────────────────────────────────────────────────────
//  Content stack init
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::initContentStack()
{
    // showDashboardPage();

    // Insert a dashboard placeholder at index 0
    // page_task_config (from .ui) shifts to index kSettingsPage = 1
    // m_dashboardPage = new QWidget(this);
    // m_dashboardPage->setStyleSheet(QString("background: %1;").arg(ptn::BG));
    // auto *dashLayout = new QVBoxLayout(m_dashboardPage);
    // dashLayout->setAlignment(Qt::AlignCenter);
    // auto *dashLbl = new QLabel(tr("Dashboard"), m_dashboardPage);
    // QFont f = dashLbl->font();
    // f.setPointSize(12);
    // dashLbl->setFont(f);
    // dashLbl->setAlignment(Qt::AlignCenter);
    // dashLbl->setStyleSheet(QString("color: %1;").arg(ptn::TXT3));
    // dashLayout->addWidget(dashLbl);

    // ui->content_stack->insertWidget(kDashboardPage, m_dashboardPage);
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

    // Sync runners with the new assigned-device set.  TaskRunner will:
    //   - register + start a runner for any newly added device
    //   - unregister + stop the runner for any removed device
    if (m_localizeTask) m_localizeTask->resyncDevices();

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

        // Status dot — design tokens (red = disconnected, green = connected)
        auto *dotLbl = new QFrame(iconBox);
        dotLbl->setFixedSize(5, 5);
        dotLbl->move(13, 13);
        dotLbl->setStyleSheet(QString(
            "QFrame { border-radius: 2px; background: %1; border: 1px solid %2; }"
        ).arg(ptn::ERR, ptn::BG));
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
        nameLbl->setStyleSheet(QString("color: %1;").arg(ptn::TXT2));
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

    // Empty state — design-token muted text
    if (!anyDevice) {
        auto *emptyLbl = new QLabel(tr("No devices assigned.\nUse [+] to add."),
                                    m_devListWidget);
        emptyLbl->setAlignment(Qt::AlignCenter);
        QFont f = emptyLbl->font();
        f.setPointSizeF(8.5);
        emptyLbl->setFont(f);
        emptyLbl->setWordWrap(true);
        emptyLbl->setStyleSheet(QString("color: %1;").arg(ptn::TXT4));
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
    // Lazy-construct the patterns widget the first time the user clicks here.
    // This matches the design handoff (`TaskWorkspace.jsx → "Patterns tab"`),
    // and keeps the patterns widget cheap when never viewed.
    if (!m_dashboardPage) {
        if (m_localizeTask) {
            m_dashboardPage = new LocalizationDashboardWidget(m_task, nullptr, this);
        } else {
            // Fallback: empty placeholder so the stack index is stable.
            m_dashboardPage = new QWidget(this);
            m_dashboardPage->setStyleSheet(QString("background: %1;").arg(ptn::BG));
        }
        // Insert at fixed index kPatternsPage (= 2). ui pages and dashboard
        // are already at 0/1, so this becomes index 2 on first creation.
        ui->content_stack->insertWidget(kDashboardPage, m_dashboardPage);
    }

    m_activeDeviceId.clear();
    ui->content_stack->setCurrentIndex(kDashboardPage);
    ui->btn_nav_dashboard->setChecked(true);
    refreshNavItemStyles();
    updateBreadcrumb(tr("Dashboard"), QColor(ptn::ACC));
    // populateBrowser();
    ui->wg_property_browser->setVisible(false);
}

void LocalizationTaskWidget::showSettingsPage()
{
    // Lazy-construct the patterns widget the first time the user clicks here.
    // This matches the design handoff (`TaskWorkspace.jsx → "Patterns tab"`),
    // and keeps the patterns widget cheap when never viewed.
    if (!m_settingPage) {
        if (m_localizeTask) {
            m_settingPage = new LocalizationSettingWidget(m_task, nullptr, this);
        } else {
            // Fallback: empty placeholder so the stack index is stable.
            m_settingPage = new QWidget(this);
            m_settingPage->setStyleSheet(QString("background: %1;").arg(ptn::BG));
        }
        // Insert at fixed index kPatternsPage (= 2). ui pages and dashboard
        // are already at 0/1, so this becomes index 2 on first creation.
        ui->content_stack->insertWidget(kSettingsPage, m_settingPage);
    }

    m_activeDeviceId.clear();
    ui->content_stack->setCurrentIndex(kSettingsPage);
    ui->btn_nav_settings->setChecked(true);
    refreshNavItemStyles();
    updateBreadcrumb(tr("Settings"), QColor(ptn::TXT2));
    // populateBrowser();
    ui->wg_property_browser->setVisible(false);
}

// ──────────────────────────────────────────────────────────────────────────────
//  Patterns page (LocalizationTask only)
// ──────────────────────────────────────────────────────────────────────────────
void LocalizationTaskWidget::showPatternsPage()
{
    // Lazy-construct the patterns widget the first time the user clicks here.
    // This matches the design handoff (`TaskWorkspace.jsx → "Patterns tab"`),
    // and keeps the patterns widget cheap when never viewed.
    if (!m_patternsPage) {
        if (m_localizeTask) {
            m_patternsPage = new LocalizationPatternsWidget(m_task, nullptr, this);
        } else {
            // Fallback: empty placeholder so the stack index is stable.
            m_patternsPage = new QWidget(this);
            m_patternsPage->setStyleSheet(QString("background: %1;").arg(ptn::BG));
        }
        // Insert at fixed index kPatternsPage (= 2). ui pages and dashboard
        // are already at 0/1, so this becomes index 2 on first creation.
        ui->content_stack->insertWidget(kPatternsPage, m_patternsPage);
    }

    m_activeDeviceId.clear();
    ui->content_stack->setCurrentWidget(m_patternsPage);
    ui->btn_nav_patterns->setChecked(true);
    refreshNavItemStyles();
    updateBreadcrumb(tr("Patterns"), QColor(ptn::ACC));

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

    ui->scrollAreaWidgetContents->setMinimumSize(page->minimumSizeHint());
    ui->content_stack->setMinimumSize(page->minimumSizeHint());
}

void LocalizationTaskWidget::onDeviceNavClicked(const QString &deviceId)
{
    showDeviceConfigPage(deviceId);
}

void LocalizationTaskWidget::refreshNavItemStyles()
{
    // Per design handoff (TaskWorkspace.jsx → "Device list items"):
    //   active: bg `{color}15`, border `1px solid {color}44`, name color #dce8f5
    //   normal: transparent + hover bg `#2a2d2e`, name color ptn::TXT2
    for (auto it = m_navItems.cbegin(); it != m_navItems.cend(); ++it) {
        QFrame *item   = it.value();
        bool    active = (it.key() == m_activeDeviceId);
        QColor  accent = colorForDeviceId(it.key());

        if (active) {
            item->setStyleSheet(QString(
                "QFrame {"
                "  border: 1px solid %1;"
                "  background-color: rgba(%2, %3, %4, 28);"
                "  border-radius: 5px;"
                "}")
                .arg(accent.name() + "66")
                .arg(accent.red()).arg(accent.green()).arg(accent.blue()));

            if (auto *lbl = item->findChild<QLabel*>("devNavName"))
                lbl->setStyleSheet(QString(
                    "color: %1; font-weight: bold;"
                ).arg(ptn::TXT));   // primary text on active
        } else {
            item->setStyleSheet(QString(
                "QFrame {"
                "  border: 1px solid transparent;"
                "  background: transparent;"
                "  border-radius: 5px;"
                "}"
                "QFrame:hover { background: %1; }"
            ).arg(ptn::SURF2));     // design-token hover surface
            if (auto *lbl = item->findChild<QLabel*>("devNavName"))
                lbl->setStyleSheet(QString("color: %1;").arg(ptn::TXT2));
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

    // ── Resolve the typed runner from TaskRunner ─────────────────────────
    // The Widget never creates a worker / thread.  TaskRunner owns the
    // runner (and its QThread); we just look it up here and inject it.
    auto *taskRunner = m_localizeTask->taskRunner();
    vc::runtime::IDeviceRunner *runner =
        taskRunner ? taskRunner->runnerFor(deviceId) : nullptr;

    // device widget factory
    QWidget *page = nullptr;
    switch (device->deviceType()) {
    case vc::device::DeviceType::Camera: {
        auto *camRunner = qobject_cast<vc::runtime::CameraRunner *>(runner);
        page = new BaslerCameraWidget(device, camRunner, nullptr, this);
        break;
    }
    case vc::device::DeviceType::PLC: {
        auto *plcRunner = qobject_cast<vc::runtime::PlcRunner *>(runner);
        page = new McProtocolDeviceWidget(device, plcRunner, nullptr, this);
        break;
    }
    case vc::device::DeviceType::VisionOutput: {
        auto *outputRunner = qobject_cast<vc::runtime::VisionOutputRunner *>(runner);
        page = new VisionOutputDeviceWidget(device, outputRunner, nullptr, this);
        break;
    }
    default: {
        page = new QWidget(this);
        page->setStyleSheet(QString("background: %1;").arg(ptn::BG));
        auto *l   = new QVBoxLayout(page);
        auto *lbl = new QLabel(tr("No configuration panel available for this device."), page);
        lbl->setAlignment(Qt::AlignCenter);
        lbl->setStyleSheet(QString("color: %1; font-size: 11pt;").arg(ptn::TXT3));
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
    // Default: design-token muted text color
    if (!m_localizeTask || !m_localizeTask->project()) return QColor(ptn::TXT2);
    auto dev = m_localizeTask->project()->deviceById(deviceId);
    if (!dev) return QColor(ptn::TXT2);
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
    QWidget *w = m_devicePages.value(id, nullptr);
    if (w) {
        IDeviceWidget *dw = qobject_cast<IDeviceWidget*>(w);
        if (dw) {
            changePropertyBrowserWidget(dw->getPropertyBrowser());
        } else {
            changePropertyBrowserDefault();
        }
    }
}

void LocalizationTaskWidget::saveConfig()
{
    if (m_localizeTask) m_localizeTask->setTaskLocalizeConfig(m_config);
}
