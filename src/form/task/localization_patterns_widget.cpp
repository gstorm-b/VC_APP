#include "localization_patterns_widget.h"
#include "ui_localization_patterns_widget.h"

#include "form/pattern/pattern_manager_dialog.h"
#include "form/pattern/pattern_theme.h"
#include "utils/theme_manager.h"
#include "form/pattern/add_pattern_wizard.h"
#include "form/pattern/edit_pattern_wizard.h"
#include "widgets/qtpropertybrowser/qtvariantproperty.h"
// #include "form/pattern/pattern_setting_panel.h"
// #include "widgets/property_browser/prop_spec.h"

#include <QTableWidget>
#include <QHeaderView>

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QSignalBlocker>
#include <QFileInfo>
#include <QDateTime>
#include <QPainter>
#include <QToolButton>
#include <QPushButton>
#include <QComboBox>
#include <QCheckBox>
#include <QFile>
#include <QLabel>
#include <QStackedWidget>
#include <QFrame>
#include <QSplitter>
#include <QHash>
#include <QPair>
#include <QMetaType>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/types.hpp>

namespace {

// State pill stylesheet templates ─ identical layout, different accent.
const char* kPillBase =
    "QLabel { color: %1; background-color: %2; border-radius: 13px; "
    "padding: 3px 10px; font: 600 9pt \"Segoe UI\"; }";

QString pillStyle(const QString &fg, const QString &bg) {
    return QString(kPillBase).arg(fg, bg);
}

inline QString resultMessage(const mtc::ManagerResult &r) {
    return QString::fromStdWString(r.error);
}

} // anonymous

// ── Image conversion helper ──────────────────────────────────────────────────

QPixmap LocalizationPatternsWidget::matToPixmap(const cv::Mat &mat) {
    if (mat.empty()) return {};

    QImage img;
    if (mat.type() == CV_8UC1) {
        img = QImage(mat.data, mat.cols, mat.rows,
                     static_cast<int>(mat.step),
                     QImage::Format_Grayscale8).copy();
    } else if (mat.type() == CV_8UC3) {
        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
        img = QImage(rgb.data, rgb.cols, rgb.rows,
                     static_cast<int>(rgb.step),
                     QImage::Format_RGB888).copy();
    } else if (mat.type() == CV_8UC4) {
        cv::Mat rgba;
        cv::cvtColor(mat, rgba, cv::COLOR_BGRA2RGBA);
        img = QImage(rgba.data, rgba.cols, rgba.rows,
                     static_cast<int>(rgba.step),
                     QImage::Format_RGBA8888).copy();
    } else {
        return {};
    }
    return QPixmap::fromImage(img);
}

static const QList<PropSpec<mtc::MatchGroupConfig>> kMatchGroupSpecs = {
    { "groupName",       "Group name",           "Group defined name.",
     QMetaType::QString, "",   "",   "",  -1, false,
     [](const mtc::MatchGroupConfig &c){ return QVariant(QString::fromStdWString(c.m_groupName)); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){ c.m_groupName = v.toString().toStdWString(); } },

    { "groupNumber",
     "Group Number",
     "Group defined number.",
     QMetaType::Int, 1, 32, 1,  1, false,
     [](const mtc::MatchGroupConfig &c){ return QVariant(c.m_groupIndex); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){ c.m_groupIndex = v.toInt(); } },

    { "groupPickingBoxSizeWidth",
     "Picking Box Size (Width)",
     "Picking box condition.",
     QMetaType::Double, 0.0, 100000.0, 1.0,  2, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingBoxSize.width); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingBoxSize.width = v.toDouble(); } },

    { "groupPickingBoxSizeHeight",
     "Picking Box Size (Height)",
     "Picking box condition.",
     QMetaType::Double, 0.0, 100000.0, 1.0,  2, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingBoxSize.height); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingBoxSize.height = v.toDouble(); } },

    { "groupPickingBoxDistance",
     "Picking Box Distance",
     "Minimum distance between two picking boxes.",
     QMetaType::Double, 0.0, 100000.0, 1.0,  2, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingBoxDistance); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingBoxDistance = v.toDouble(); } },

    { "groupPickingBoxAngle",
     "Picking Box Angle",
     "Orientation of the picking box (degrees).",
     QMetaType::Double, -360.0, 360.0, 0.1,  2, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingBoxAngle); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingBoxAngle = v.toDouble(); } },

    { "groupPickingOffsetX",
     "Picking Offset (X)",
     "X-axis offset applied at picking time.",
     QMetaType::Double, -100000.0, 100000.0, 0.1,  3, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingOffset.x); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingOffset.x = v.toFloat(); } },

    { "groupPickingOffsetY",
     "Picking Offset (Y)",
     "Y-axis offset applied at picking time.",
     QMetaType::Double, -100000.0, 100000.0, 0.1,  3, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingOffset.y); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingOffset.y = v.toFloat(); } },

    { "groupPickingOffsetZ",
     "Picking Offset (Z)",
     "Z-axis offset applied at picking time.",
     QMetaType::Double, -100000.0, 100000.0, 0.1,  3, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_pickingOffset.z); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_pickingOffset.z = v.toFloat(); } },

    { "groupLowWorkpieceRatio",
     "Low Workpiece Ratio",
     "Edge-based match: ratio used to detect low / partially visible workpieces.",
     QMetaType::Double, 0.0, 100.0, 0.1,  3, false,
     [](const mtc::MatchGroupConfig &c){
         return QVariant(c.m_lowWorkpieceRatio); },
     [](mtc::MatchGroupConfig &c, const QVariant &v){
         c.m_lowWorkpieceRatio = v.toDouble(); } },
};

// ── Construction ─────────────────────────────────────────────────────────────

LocalizationPatternsWidget::LocalizationPatternsWidget(
        std::shared_ptr<vc::model::ITask> task,
        ads::CDockWidget *dock, QWidget *parent)
    : ITaskWidget(task, dock, parent),
      ui(new Ui::LocalizationPatternsWidget) {

    ui->setupUi(this);
    initWidget();
}

LocalizationPatternsWidget::~LocalizationPatternsWidget() {
    clearPropertyBrowserState();
    delete ui;
}

void LocalizationPatternsWidget::loadConfigToTask()   {}
void LocalizationPatternsWidget::loadConfigToWidget() {}

// ── initWidget ───────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::initWidget() {
    setupThemeReload(QStringLiteral(":/styles/localization_patterns_widget_dark.qss"),
                     QStringLiteral(":/styles/localization_patterns_widget_light.qss"));

    // Refresh the right-pane section title so it matches what the panel shows.
    if (ui->label_property_title) {
        ui->label_property_title->setText(tr("  PATTERN CONFIGURATION"));
    }

    // splitter ratios: monitor | tree+thumb (library pane) | inspector
    ui->splitter_main->setStretchFactor(0, 6);
    ui->splitter_main->setStretchFactor(1, 3);
    ui->splitter_main->setStretchFactor(2, 3);

    // splitter ratios:  library tree | thumb widget
    ui->splitter_thumb->setStretchFactor(0, 6);
    ui->splitter_thumb->setStretchFactor(1, 4);

    // ── Pattern thumbnail scene — same QGraphicsView (now under the tree) ──
    m_thumbScene  = new QGraphicsScene(this);
    m_thumbPixmap = m_thumbScene->addPixmap(QPixmap{});
    if (ui->imageView_PatternThumb) {
        ui->imageView_PatternThumb->setScene(m_thumbScene);
        ui->imageView_PatternThumb->setRenderHint(QPainter::SmoothPixmapTransform);
    }

    // Bind to the task
    if (m_task) {
        m_localizeTask   = static_cast<vc::model::TaskLocalization*>(m_task.get());
        m_config         = m_localizeTask->taskLocalizeConfig();
        m_patternManager = m_localizeTask->patternManager();
    }

    // Camera-image dialog (legacy capture flow used by the new wizard)
    m_addPatternImageDialog = new AddPatternImageDialog(this);
    connect(m_addPatternImageDialog, &AddPatternImageDialog::requestImage,
            this, &LocalizationPatternsWidget::requestCameraImage);

    // init property widget
    if (ui->wg_property_browser) {
        initPropertyBrowser(ui->wg_property_browser);
    }

    // Property adapter for pattern-level config
    m_configAdapter = new mtc::MatchConfigPropertyAdapter(m_variantManager, this);
    connect(m_configAdapter, &mtc::MatchConfigPropertyAdapter::configModified,
            this, &LocalizationPatternsWidget::onPatternConfigModified);

    wireToolbar();
    wireTree();
    wireManagerSignals();
    // Wire the property-browser's manager-level valueChanged → group-property
    // commit slot, and seed the browser with the group-settings group.  Must
    // run after m_configAdapter has been constructed (above) so both handlers
    // observe the same QtVariantPropertyManager instance.
    wirePropertyBrowser();

    // Build & install the Result Table below the KPI strip (Pane 1 bottom).
    // Mirrors `MonitorPane > Result Table` from PatternManager.jsx.
    buildResultTable();

    rebuildGroupCombo();
    updateGroupsCount();
    setState(State::Idle);
    setMonitorPage(0);

    // install PatternSettingPanel inside ui->wg_property_browser
    // (the empty placeholder host inside the right inspector pane).
/*    m_settingPanel = new PatternSettingPanel(this);
    m_settingPanel->setShowInlineThumbnail(false);   */// dedicated thumb is below tree

    // if (ui->wg_property_browser) {
    //     // initPropertyBrowser(nullptr) means no layout was created on the
    //     // placeholder — install one ourselves to host the panel.
    //     auto *propLay = ui->wg_property_browser->layout();
    //     if (!propLay) {
    //         propLay = new QVBoxLayout(ui->wg_property_browser);
    //         propLay->setContentsMargins(0, 0, 0, 0);
    //         propLay->setSpacing(0);
    //     }
    //     propLay->addWidget(m_settingPanel);
    // }

    // Wire setting-panel signals to host actions.
    // connect(m_settingPanel, &PatternSettingPanel::learnRequested,
    //         this, &LocalizationPatternsWidget::onTriggerCameraClicked);
    // connect(m_settingPanel, &PatternSettingPanel::openImageRequested,
    //         this, &LocalizationPatternsWidget::onChooseImageClicked);
    // connect(m_settingPanel, &PatternSettingPanel::patternFieldChanged,
    //         this, &LocalizationPatternsWidget::onSettingPanelPatternFieldChanged);
    // connect(m_settingPanel, &PatternSettingPanel::groupFieldChanged,
    //         this, &LocalizationPatternsWidget::onSettingPanelGroupFieldChanged);
}

// ── Wiring helpers ───────────────────────────────────────────────────────────

void LocalizationPatternsWidget::wireToolbar() {
    connect(ui->btn_trigger_camera, &QToolButton::clicked,
            this, &LocalizationPatternsWidget::onTriggerCameraClicked);
    connect(ui->btn_choose_image, &QToolButton::clicked,
            this, &LocalizationPatternsWidget::onChooseImageClicked);
    connect(ui->btn_set_workspace, &QToolButton::clicked,
            this, &LocalizationPatternsWidget::onSetWorkspaceClicked);
    connect(ui->btn_run_match, &QToolButton::clicked,
            this, &LocalizationPatternsWidget::onRunMatchingClicked);

    connect(ui->btn_view_raw, &QPushButton::clicked,
            this, &LocalizationPatternsWidget::onShowRawView);
    connect(ui->btn_view_result, &QPushButton::clicked,
            this, &LocalizationPatternsWidget::onShowResultView);

    connect(ui->comboBox_pattern_group,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &LocalizationPatternsWidget::onActiveGroupChanged);

    // Camera combo: rebuild whenever the task's assigned-device list changes,
    // then seed it once with the current state.  The host is expected to
    // listen for requestCameraImage(id) and feed an image back via
    // setCameraImage(), so the widget itself does not touch the camera here.
    if (m_localizeTask) {
        connect(m_localizeTask, &vc::model::ITask::devicesChanged,
                this, &LocalizationPatternsWidget::rebuildCameraCombo);
    }
    rebuildCameraCombo();

    ui->btn_run_match->setEnabled(false);
}

void LocalizationPatternsWidget::wireTree() {
    auto *tree = ui->treeWidget_patternEditor;
    connect(tree, &PatternTreeWidget::groupClicked,
            this, &LocalizationPatternsWidget::onTreeGroupClicked);
    connect(tree, &PatternTreeWidget::patternClicked,
            this, &LocalizationPatternsWidget::onTreePatternClicked);

    connect(tree, &PatternTreeWidget::addGroupRequested,
            this, &LocalizationPatternsWidget::onTreeAddGroupRequested);
    connect(tree, &PatternTreeWidget::addPatternRequested,
            this, &LocalizationPatternsWidget::onTreeAddPatternRequested);
    connect(tree, &PatternTreeWidget::deleteGroupRequested,
            this, &LocalizationPatternsWidget::onTreeDeleteGroupRequested);
    connect(tree, &PatternTreeWidget::deletePatternRequested,
            this, &LocalizationPatternsWidget::onTreeDeletePatternRequested);

    connect(tree, &PatternTreeWidget::groupsChanged,
            this, &LocalizationPatternsWidget::onTreeGroupsChanged);
    connect(tree, &PatternTreeWidget::groupChanged,
            this, &LocalizationPatternsWidget::onTreeGroupChanged);
    connect(tree, &PatternTreeWidget::patternChanged,
            this, &LocalizationPatternsWidget::onTreePatternChanged);
}

void LocalizationPatternsWidget::wireManagerSignals() {
    if (!m_patternManager) return;

    // ── Initial population ──────────────────────────────────────────────────
    // The tree is empty when widget is constructed.  Push current manager
    // state into it so existing groups/patterns (loaded from project JSON)
    // appear immediately — without this, the user sees an empty library.
    rebuildTreeFromManager();

    // ── Group added ────────────────────────────────────────────────────────
    connect(m_patternManager, &mtc::PatternGroupManager::groupAdded,
            this, [this] (std::shared_ptr<mtc::MatchGroup> group) {
        if (!group) return;
        MatchGroupConfig gr;
        gr.name    = QString::fromStdWString(group->name());
        gr.number  = group->number();
        // // Mirror pre-existing patterns so the tree shows complete state.
        for (const auto &p : group->patterns()) {
            MatchPatternConfig pc;
            pc.name        = QString::fromStdWString(p->name());
            pc.number      = p->number();
            pc.threshScore = p->config().m_minScore;
            pc.thumbnail   = matToPixmap(p->config().m_rawImage);
            gr.patterns.append(pc);
        }
        ui->treeWidget_patternEditor->addGroup(gr);
        rebuildGroupCombo();
        updateGroupsCount();
    });

    // ── Group removed ──────────────────────────────────────────────────────
    connect(m_patternManager, &mtc::PatternGroupManager::groupRemoved,
            this, [this] (const mtc::MatchGroupConfig &removed) {
        // Mirror the removal in the tree so the row disappears.
        ui->treeWidget_patternEditor->removeGroup(removed.m_groupIndex);

        // If the removed group was selected, clear selection panel.
        if (m_selectedGroupIndex == removed.m_groupIndex) clearSelection();

        rebuildGroupCombo();
        updateGroupsCount();
    });

    // ── Group changed (rename/renumber/picking-box edit) ───────────────────
    // Fires for any successful PatternGroupManager group mutation.  The
    // signal does not carry the *previous* identity of the group, so the
    // safest refresh is a full tree rebuild (it preserves expansion state).
    connect(m_patternManager, &mtc::PatternGroupManager::groupChanged,
            this, [this] (std::shared_ptr<mtc::MatchGroup> group, const QString &) {
        if (!group) return;

        // Keep our cached selection in sync if the currently-bound group
        // was the one that changed.  The pointer is stable across edits
        // (setConfig updates in place), so identity tracking via raw
        // pointer is reliable here.
        if (m_boundMatchGroup == group.get())
            m_selectedGroupIndex = group->number();

        rebuildTreeFromManager();
        rebuildGroupCombo();

        // rebuildGroupCombo() restores selection by previous currentData()
        // (the old group number), which won't match after a renumber.
        // Re-select explicitly by the cached (possibly updated) number.
        const int idx = ui->comboBox_pattern_group->findData(m_selectedGroupIndex);
        if (idx >= 0) {
            QSignalBlocker b(ui->comboBox_pattern_group);
            ui->comboBox_pattern_group->setCurrentIndex(idx);
        }
    });

    // ── Pattern added ──────────────────────────────────────────────────────
    connect(m_patternManager, &mtc::PatternGroupManager::patternAdded,
            this, [this] (mtc::MatchGroup *group, mtc::MatchPattern *pattern) {
        if (!group || !pattern) return;
        MatchPatternConfig p;
        p.name        = QString::fromStdWString(pattern->name());
        p.number      = pattern->number();
        p.threshScore = pattern->config().m_minScore;
        p.thumbnail   = matToPixmap(pattern->config().m_rawImage);
        ui->treeWidget_patternEditor->addPattern(group->number(), p);
        updateGroupsCount();
    });

    // ── Pattern removed ────────────────────────────────────────────────────
    connect(m_patternManager, &mtc::PatternGroupManager::patternRemoved,
            this, [this] (mtc::MatchGroup *group, const mtc::MatchPatternConfig &removed) {
        if (group)
            ui->treeWidget_patternEditor->removePattern(group->number(),
                                                        removed.m_patternIndex);
        // Clear selection if the removed pattern was selected.
        if (group
            && m_selectedGroupIndex   == group->number()
            && m_selectedPatternIndex == removed.m_patternIndex) {
            // Fall back to group-only selection
            m_selectedPatternIndex = -1;
            // if (m_settingPanel) m_settingPanel->setSelection(group, nullptr);
        }
        updateGroupsCount();
    });

    // ── Pattern changed (config edit / rename / renumber / image) ──────────
    // Fires for any successful PatternGroupManager pattern mutation.  Keep
    // the cached selection identity in sync (number may have changed) and
    // refresh the tree + thumbnail.
    connect(m_patternManager, &mtc::PatternGroupManager::patternChanged,
            this, [this] (mtc::MatchGroup *group, mtc::MatchPattern *pattern,
                          const QString &) {
        if (!group || !pattern) return;

        if (m_boundPattern == pattern) {
            m_selectedPatternIndex = pattern->number();
            updatePatternThumb(pattern);
        }

        rebuildTreeFromManager();
    });
}

// Pull the full library state from the manager and rebuild the tree from
// scratch.  Used at init and after bulk re-orderings.
void LocalizationPatternsWidget::rebuildTreeFromManager() {
    if (!m_patternManager || !ui->treeWidget_patternEditor) return;

    QList<MatchGroupConfig> rows;
    for (auto group : m_patternManager->groups()) {
        if (!group) continue;
        MatchGroupConfig gr;
        gr.name   = QString::fromStdWString(group->name());
        gr.number = group->number();
        for (const auto &p : group->patterns()) {
            MatchPatternConfig pc;
            pc.name        = QString::fromStdWString(p->name());
            pc.number      = p->number();
            pc.threshScore = p->config().m_minScore;
            pc.thumbnail   = matToPixmap(p->config().m_rawImage);
            gr.patterns.append(pc);
        }
        rows.append(gr);
    }
    ui->treeWidget_patternEditor->setGroups(rows);
}

void LocalizationPatternsWidget::wirePropertyBrowser() {
    // Manager-level valueChanged: pattern-level edits are absorbed by the
    // adapter (it ignores anything not in its key map).  This handler is
    // only for the group-level properties that we own ourselves.
    connect(m_variantManager, &QtVariantPropertyManager::valueChanged,
            this, &LocalizationPatternsWidget::onPropertyValueChanged);

    rebuildPropertyBrowser();
}

// ── Property browser construction ────────────────────────────────────────────

void LocalizationPatternsWidget::clearPropertyBrowserState() {
    if (m_variantEditor) {
        m_variantEditor->clear();
    }

    if (m_configAdapter) {
        m_configAdapter->bind(nullptr);
    }

    m_groupProps.clear();
    m_groupPropKeys.clear();
    delete m_groupVariant;
    m_groupVariant = nullptr;
}

void LocalizationPatternsWidget::buildGroupProperties() {
    m_groupVariant = m_variantManager->addProperty(
        QtVariantPropertyManager::groupTypeId(), tr("Group Settings"));

    m_groupPropKeys.clear();
    m_groupProps.clear();
    auto sub = PropSpecHelper::buildGroup<mtc::MatchGroupConfig>(m_variantManager, m_groupVariant,
                                          kMatchGroupSpecs, m_workingGroupConfig, m_groupPropKeys);
    for (auto it = sub.cbegin(); it != sub.cend(); ++it)
        m_groupProps[it.key()] = it.value();
}

void LocalizationPatternsWidget::rebuildPropertyBrowser() {
    if (!m_variantManager || !m_variantEditor || !m_configAdapter) return;

    // Tear down the current property tree while the shared manager is still
    // alive. Both the adapter and the group block own only the root groups;
    // child properties are released by QtProperty when the root is deleted.
    clearPropertyBrowserState();

    // 4. Rebuild group block.
    if (m_boundMatchGroup) {
        m_workingGroupConfig = m_boundMatchGroup->config();
        buildGroupProperties();
    }

    // 5. Rebuild pattern block via the adapter.
    if (m_boundPattern) {
        m_configAdapter->bind(&m_workingPatternCfg);
    }

    // 6. Mount into the browser.
    if (m_groupVariant)
        m_variantEditor->addProperty(m_groupVariant);
    for (auto *p : m_configAdapter->rootProperties())
        m_variantEditor->addProperty(p);
}

void LocalizationPatternsWidget::bindPatternToBrowser(mtc::MatchPattern *pattern) {
    m_boundPattern      = pattern;
    m_workingPatternCfg = pattern ? pattern->config() : mtc::MatchPatternConfig{};
    rebuildPropertyBrowser();
}

void LocalizationPatternsWidget::unbindPattern() {
    m_boundPattern = nullptr;
    m_workingPatternCfg = mtc::MatchPatternConfig{};
    rebuildPropertyBrowser();
}

// ── Selection ────────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::selectGroup(int groupIndex) {
    m_selectedGroupIndex   = groupIndex;
    m_selectedPatternIndex = -1;
    m_boundPattern = nullptr;
    m_boundMatchGroup = nullptr;
    updatePatternThumb(nullptr);

    // Push selection into the under-tree PatternSettingPanel
    if (m_patternManager) {
        auto group = m_patternManager->findGroupByNumber(groupIndex);
        m_boundMatchGroup = group.get();
        m_workingGroupConfig = m_boundMatchGroup->config();
    }

    rebuildPropertyBrowser();
}

void LocalizationPatternsWidget::selectPattern(int groupIndex, int patternIndex) {
    m_selectedGroupIndex   = groupIndex;
    m_selectedPatternIndex = patternIndex;

    if (!m_patternManager) return;
    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) return;

    auto *pattern = group->findPatternByNumber(patternIndex);
    if (pattern) {
        // build group
        m_boundMatchGroup = group.get();
        m_workingGroupConfig = m_boundMatchGroup->config();

        // build pattern
        bindPatternToBrowser(pattern);
        updatePatternThumb(pattern);
    }
}

void LocalizationPatternsWidget::clearSelection() {
    m_selectedGroupIndex   = -1;
    m_selectedPatternIndex = -1;
    m_boundMatchGroup = nullptr;
    m_workingGroupConfig = mtc::MatchGroupConfig{};
    unbindPattern();
    updatePatternThumb(nullptr);
}

void LocalizationPatternsWidget::updatePatternThumb(mtc::MatchPattern *pattern) {
    if (!pattern || pattern->isImageEmpty()) {
        m_thumbPixmap->setPixmap(QPixmap{});
        ui->label_pattern_caption->setText(tr("No pattern selected"));
        return;
    }

    cv::Mat img = pattern->getImageWithPickPosition();
    if (img.empty()) img = pattern->getRawImage();

    QPixmap pm = matToPixmap(img);
    m_thumbPixmap->setPixmap(pm);
    m_thumbScene->setSceneRect(pm.rect());
    ui->imageView_PatternThumb->fitInView(m_thumbPixmap, Qt::KeepAspectRatio);

    ui->label_pattern_caption->setText(
        tr("%1   |   %2 × %3 px   |   min score %4")
            .arg(QString::fromStdWString(pattern->name()))
            .arg(pm.width()).arg(pm.height())
            .arg(pattern->config().m_minScore, 0, 'f', 2));

    // Forward to the new under-tree PatternSettingPanel — its own thumbnail
    // QLabel is the user-visible one (the legacy QGraphicsView is hidden
    // inside wg_thumb_container after the design refactor).
    // if (m_settingPanel) m_settingPanel->setPatternThumbnail(pm);
}

// ── Camera combo ────────────────────────────────────────────────────
void LocalizationPatternsWidget::rebuildCameraCombo() {
    if (!m_localizeTask || !ui->comboBox_camera) return;

    QSignalBlocker blocker(ui->comboBox_camera);

    // Remember current selection so we can restore it if still present.
    const QString previousId = ui->comboBox_camera->currentData().toString();

    ui->comboBox_camera->clear();

    // Pull only camera-typed devices from the task's assigned list.
    const auto cameras =
        m_localizeTask->assignedDevicesOfType(vc::device::DeviceType::Camera);
    for (const auto &dev : cameras) {
        if (!dev) continue;
        // Display: "<name>  ·  <id>" — id kept as itemData for the trigger.
        ui->comboBox_camera->addItem(
            tr("%1").arg(dev->name()),
            dev->id());
    }

    // Restore previous selection if it still exists.
    if (!previousId.isEmpty()) {
        const int idx = ui->comboBox_camera->findData(previousId);
        if (idx >= 0) ui->comboBox_camera->setCurrentIndex(idx);
    }

    // Trigger button is meaningful only when at least one camera is assigned.
    const bool hasCamera = ui->comboBox_camera->count() > 0;
    if (ui->btn_trigger_camera)
        ui->btn_trigger_camera->setEnabled(hasCamera);
}

// ── Group combo ──────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::rebuildGroupCombo() {
    if (!m_patternManager) return;

    QSignalBlocker blocker(ui->comboBox_pattern_group);
    const int previous = ui->comboBox_pattern_group->currentData().toInt();

    ui->comboBox_pattern_group->clear();
    for (auto group : m_patternManager->groups()) {
        const QString name = QString::fromStdWString(group->name());
        const int     num  = group->number();
        ui->comboBox_pattern_group->addItem(
            tr("[%1]  %2").arg(num).arg(name), num);
    }

    // Restore previous selection if still present.
    int restoreIndex = ui->comboBox_pattern_group->findData(previous);
    if (restoreIndex < 0 && ui->comboBox_pattern_group->count() > 0)
        restoreIndex = 0;

    if (restoreIndex >= 0)
        ui->comboBox_pattern_group->setCurrentIndex(restoreIndex);

    // Run-match button is only meaningful with a non-empty group + image.
    const bool hasGroup = ui->comboBox_pattern_group->count() > 0;
    ui->btn_run_match->setEnabled(hasGroup && !m_currentImage.empty());
}

void LocalizationPatternsWidget::onActiveGroupChanged(int comboIndex) {
    if (comboIndex < 0) return;
    const int groupNumber =
        ui->comboBox_pattern_group->itemData(comboIndex).toInt();
    selectGroup(groupNumber);
}

// ── Tree click handlers ─────────────────────────────────────────────────────

void LocalizationPatternsWidget::onTreeGroupClicked(int groupIndex,
                                                    const MatchGroupConfig &) {
    selectGroup(groupIndex);

    // Sync the active-group combo so user always sees what they clicked.
    int idx = ui->comboBox_pattern_group->findData(groupIndex);
    if (idx >= 0) {
        QSignalBlocker b(ui->comboBox_pattern_group);
        ui->comboBox_pattern_group->setCurrentIndex(idx);
    }
}

void LocalizationPatternsWidget::onTreePatternClicked(int groupIndex,
                                                      int patternIndex,
                                                      const MatchPatternConfig &) {
    selectPattern(groupIndex, patternIndex);

    int idx = ui->comboBox_pattern_group->findData(groupIndex);
    if (idx >= 0) {
        QSignalBlocker b(ui->comboBox_pattern_group);
        ui->comboBox_pattern_group->setCurrentIndex(idx);
    }
}

// ── Tree intent handlers ────────────────────────────────────────────────────

void LocalizationPatternsWidget::onTreeAddGroupRequested() {
    if (!m_patternManager) return;

    QString title = tr("Input new Pattern Group information");
    AddGroupDialog dialog(title, this);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString name  = dialog.getGroupName();
    const int     index = dialog.getGroupIndex();

    if (m_patternManager->containsGroupName(name)) {
        QMessageBox::warning(this, tr("Duplicated group name"),
                             tr("Pattern group name already exists."));
        return;
    }
    if (m_patternManager->containsGroupNumber(index)) {
        QMessageBox::warning(this, tr("Duplicated group index"),
                             tr("Pattern group index already exists."));
        return;
    }

    mtc::MatchGroupConfig cfg;
    cfg.m_groupName  = name.toStdWString();
    cfg.m_groupIndex = index;
    auto r = m_patternManager->addGroup(cfg);
    if (!r) {
        QMessageBox::warning(this, tr("Add group failed"),
                             resultMessage(r));
    }
}

void LocalizationPatternsWidget::onTreeAddPatternRequested(int groupIndex) {
    if (!m_patternManager) return;

    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) {
        QMessageBox::warning(this, tr("Add pattern error"),
                             tr("Group not found."));
        return;
    }
    const QString groupName = QString::fromStdWString(group->name());

    // Build "used" lists from existing patterns in the group so the wizard
    // can validate uniqueness inline.
    QStringList usedNames;
    QList<int>  usedNumbers;
    {
        std::vector<std::shared_ptr<mtc::MatchPattern>> pats = group->patterns();
        for (const auto &p : pats) {
            usedNames   << QString::fromStdWString(p->name());
            usedNumbers << p->number();
        }
    }

    // ── New 5-step Add Pattern wizard ─────────────────────────────────────
    // Replaces the old two-step flow (image dialog + simple AddPatternDialog).
    // Mirrors `ui_scratch/design_handoff_full_project/components/PatternWizard.jsx`.
    AddPatternWizard wiz(groupName, usedNames, usedNumbers, this);
    m_activeAddWizard = &wiz;
    // Forward camera-capture request from wizard → host (same plumbing as
    // the legacy AddPatternImageDialog).  The wizard's signal is parameterless;
    // adapt to the host signal by injecting the currently-selected camera id
    // from the toolbar combo.
    connect(&wiz, &AddPatternWizard::requestCameraImage,
            this, [this]() {
        const QString cameraId = ui->comboBox_camera
            ? ui->comboBox_camera->currentData().toString()
            : QString();
        emit requestCameraImage(cameraId);
    });

    const int wizResult = wiz.exec();
    m_activeAddWizard = nullptr;
    if (wizResult != QDialog::Accepted) return;

    cv::Mat patternImage = wiz.patternImage();
    if (patternImage.empty()) return;

    // // Apply crop if user did not check "use original frame"
    // if (!wiz.keepOriginal()) {
    //     const QRect r = wiz.cropRect();
    //     if (r.width() > 0 && r.height() > 0
    //         && r.x() >= 0 && r.y() >= 0
    //         && r.x() + r.width()  <= patternImage.cols
    //         && r.y() + r.height() <= patternImage.rows)
    //     {
    //         patternImage = patternImage(cv::Rect(r.x(), r.y(),
    //                                              r.width(), r.height())).clone();
    //     }
    // }

    mtc::MatchPatternConfig cfg;
    cfg.m_patternName  = wiz.patternName().toStdWString();
    cfg.m_patternIndex = wiz.patternNumber();
    cfg.m_rawImage     = patternImage.clone();

    auto r = m_patternManager->addPattern(groupName, cfg);
    if (!r) {
        QMessageBox::warning(this, tr("Add pattern failed"),
                             resultMessage(r));
        return;
    }

    // Push pick + box config back into the freshly created pattern
    if (auto *pat = group->findPatternByNumber(wiz.patternNumber())) {
        auto pcfg = pat->config();
        pcfg.m_pickPosition = cv::Point2f(static_cast<float>(wiz.pickX()),
                                           static_cast<float>(wiz.pickY()));
        pat->setConfig(pcfg);
    }
    // Push picking-box settings onto the group (group-level config)
    {
        mtc::MatchGroupConfig gcfg = group->config();
        gcfg.m_pickingBoxSize.width  = static_cast<float>(wiz.pickBoxW());
        gcfg.m_pickingBoxSize.height = static_cast<float>(wiz.pickBoxH());
        gcfg.m_pickingBoxDistance    = wiz.pickBoxDist();
        gcfg.m_pickingBoxAngle       = wiz.pickBoxAngle();
        // group->setConfig(gcfg);
        m_patternManager->setGroupConfig(groupName, gcfg);
    }
}

// ── Edit Pattern Wizard entry points ────────────────────────────────────────

bool LocalizationPatternsWidget::editPattern(int groupNumber, int patternNumber) {
    if (!m_patternManager) return false;

    auto group = m_patternManager->findGroupByNumber(groupNumber);
    if (!group) return false;

    auto *pat = group->findPatternByNumber(patternNumber);
    if (!pat) return false;

    const QString groupName = QString::fromStdWString(group->name());

    // Build "used" lists excluding the pattern being edited so its current
    // name/number stays valid.
    QStringList usedNames;
    QList<int>  usedNumbers;
    {
        std::vector<std::shared_ptr<mtc::MatchPattern>> pats = group->patterns();
        for (const auto &p : pats) {
            if (p->number() == patternNumber) continue;
            usedNames   << QString::fromStdWString(p->name());
            usedNumbers << p->number();
        }
    }

    // Prefill from the current saved state
    EditPatternWizard::Pattern initial;
    {
        const auto &pcfg = pat->config();
        const auto &gcfg = group->config();
        initial.name         = QString::fromStdWString(pat->name());
        initial.number       = pat->number();
        initial.pickX        = static_cast<int>(pcfg.m_pickPosition.x);
        initial.pickY        = static_cast<int>(pcfg.m_pickPosition.y);
        initial.pickBoxW     = gcfg.m_pickingBoxSize.width;
        initial.pickBoxH     = gcfg.m_pickingBoxSize.height;
        initial.pickBoxDist  = gcfg.m_pickingBoxDistance;
        initial.pickBoxAngle = gcfg.m_pickingBoxAngle;
        initial.image        = pcfg.m_rawImage.clone();
    }

    EditPatternWizard wiz(groupName, initial, usedNames, usedNumbers, this);
    if (wiz.exec() != QDialog::Accepted) return false;

    const auto out = wiz.result();

    // Apply pattern-level changes (name, number, pick position).
    // Re-fetch by old number in case the user changed it.
    auto *editTarget = group->findPatternByNumber(patternNumber);
    if (!editTarget) return false;

    auto pcfg = editTarget->config();
    pcfg.m_patternName  = out.name.toStdWString();
    pcfg.m_patternIndex = out.number;
    pcfg.m_pickPosition = cv::Point2f(static_cast<float>(out.pickX),
                                       static_cast<float>(out.pickY));
    editTarget->setConfig(pcfg);

    // Apply group-level changes (picking box).
    mtc::MatchGroupConfig gcfg = group->config();
    gcfg.m_pickingBoxSize.width  = static_cast<float>(out.pickBoxW);
    gcfg.m_pickingBoxSize.height = static_cast<float>(out.pickBoxH);
    gcfg.m_pickingBoxDistance    = out.pickBoxDist;
    gcfg.m_pickingBoxAngle       = out.pickBoxAngle;
    // group->setConfig(gcfg);
    m_patternManager->setGroupConfig(groupName, gcfg);


    return true;
}

void LocalizationPatternsWidget::editSelectedPattern() {
    if (m_selectedGroupIndex < 0 || m_selectedPatternIndex < 0) {
        QMessageBox::information(this, tr("Edit pattern"),
                                  tr("Select a pattern in the library first."));
        return;
    }
    editPattern(m_selectedGroupIndex, m_selectedPatternIndex);
}

void LocalizationPatternsWidget::onTreeDeleteGroupRequested(int groupIndex) {
    if (!m_patternManager) return;

    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) return;

    const QString name = QString::fromStdWString(group->name());
    auto reply = QMessageBox::question(
        this, tr("Delete group"),
        tr("Delete group \"%1\" and all its patterns?").arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    auto r = m_patternManager->removeGroup(name);
    if (!r) {
        QMessageBox::warning(this, tr("Delete group failed"),
                             resultMessage(r));
        return;
    }

    // Mirror the removal in the tree widget.
    // removeGroup() takes the user-defined group number (mapped internally).
    ui->treeWidget_patternEditor->removeGroup(groupIndex);

    if (m_selectedGroupIndex == groupIndex) clearSelection();
}

void LocalizationPatternsWidget::onTreeDeletePatternRequested(int groupIndex,
                                                              int patternIndex) {
    if (!m_patternManager) return;

    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) return;

    auto *pattern = group->findPatternByNumber(patternIndex);
    if (!pattern) return;

    const QString groupName = QString::fromStdWString(group->name());
    const QString patName   = QString::fromStdWString(pattern->name());

    auto reply = QMessageBox::question(
        this, tr("Delete pattern"),
        tr("Delete pattern \"%1\" from group \"%2\"?").arg(patName, groupName),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (reply != QMessageBox::Yes) return;

    auto r = m_patternManager->removePattern(groupName, patName);
    if (!r) {
        QMessageBox::warning(this, tr("Delete pattern failed"),
                             resultMessage(r));
        return;
    }

    // Mirror the removal in the tree widget.
    // removePattern() takes user group number + pattern list index.
    auto groups = ui->treeWidget_patternEditor->groups();
    for (int gi = 0; gi < groups.size(); ++gi) {
        if (groups[gi].number != groupIndex) continue;
        for (int pi = 0; pi < groups[gi].patterns.size(); ++pi) {
            if (groups[gi].patterns[pi].number == patternIndex) {
                ui->treeWidget_patternEditor->removePattern(groupIndex, pi);
                break;
            }
        }
        break;
    }

    if (m_selectedGroupIndex == groupIndex
            && m_selectedPatternIndex == patternIndex) {
        unbindPattern();
        updatePatternThumb(nullptr);
        m_selectedPatternIndex = -1;
    }
}

// Tree-state change notifications: used for live rename / renumber from the
// tree's inline editors.  We mirror those into the manager.

void LocalizationPatternsWidget::onTreeGroupsChanged(
        const QList<MatchGroupConfig> &) {

}

void LocalizationPatternsWidget::onTreeGroupChanged(int /*groupIndex*/,
                                                    const MatchGroupConfig &) {
    // Not wiring rename here yet — manager update would need both old and
    // new names.  Tree widget exposes only the new state, so a future
    // enhancement is to track previous name per row.
}

void LocalizationPatternsWidget::onTreePatternChanged(int /*groupIndex*/,
                                                      int /*patternIndex*/,
                                                      const MatchPatternConfig &) {
    // Same caveat as above.
}

// ── Toolbar ─────────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::onTriggerCameraClicked() {
    // Should not normally fire when no camera is assigned — the Trigger
    // button is disabled via rebuildCameraCombo() in that case.  Guard
    // anyway for the race where the assignment changes between the click
    // hit-test and this slot running.
    const QString cameraId = ui->comboBox_camera
        ? ui->comboBox_camera->currentData().toString()
        : QString();

    if (cameraId.isEmpty()) {
        setState(State::Warning, tr("No camera selected."));
        return;
    }

    setState(State::Busy, tr("Waiting for camera image…"));
    emit requestCameraImage(cameraId);
}

void LocalizationPatternsWidget::onChooseImageClicked() {
    const QString path = QFileDialog::getOpenFileName(
        this, tr("Open test image"),
        {}, tr("Images (*.png *.bmp *.jpg *.jpeg *.tif *.tiff)"));
    if (path.isEmpty()) return;

    cv::Mat img = cv::imread(path.toStdString(), cv::IMREAD_UNCHANGED);
    if (img.empty()) {
        setState(State::Error, tr("Failed to load image."));
        return;
    }

    setCameraImage(img);
    ui->label_status_image->setText(
        tr("Image: %1  ·  %2 × %3")
            .arg(QFileInfo(path).fileName())
            .arg(img.cols).arg(img.rows));
}

void LocalizationPatternsWidget::onSetWorkspaceClicked() {
    // ROI drawing is not yet wired in ImageViewOnly.  Surface the intent
    // clearly so the user knows what's pending rather than failing silently.
    setState(State::Warning,
             tr("Workspace selection requires the ROI editor (TBD)."));
}

void LocalizationPatternsWidget::onShowRawView() {
    setMonitorPage(0);
}

void LocalizationPatternsWidget::onShowResultView() {
    setMonitorPage(1);
}

void LocalizationPatternsWidget::onRunMatchingClicked() {
    if (m_currentImage.empty()) {
        setState(State::Warning, tr("No image loaded."));
        return;
    }
    if (m_selectedGroupIndex < 0) {
        setState(State::Warning, tr("Select an active group first."));
        return;
    }

    setState(State::Busy, tr("Matching…"));
    QApplication::processEvents();   // flush state-pill repaint

    mtc::MatchResult result;
    if (!runMatchingTest(result)) {
        setState(State::Error, tr("Matching failed (see log)."));
        return;
    }

}

// ── External image input ─────────────────────────────────────────────────────

void LocalizationPatternsWidget::setCameraImage(const cv::Mat &image) {
    if (image.empty()) {
        setState(State::Error, tr("Received empty image."));
        return;
    }

    // Route the image to whoever asked for it.

    // 1. Active Add Pattern wizard takes precedence over everything else —
    //    the user explicitly requested capture from inside the wizard.
    if (m_activeAddWizard) {
        m_activeAddWizard->setCameraImage(image);
        return;
    }

    // 2. Legacy AddPatternImage dialog (used by older flows / external code).
    if (m_addPatternImageDialog && m_addPatternImageDialog->isVisible()) {
        m_addPatternImageDialog->setMainViewImage(matToPixmap(image));
        return;
    }

    m_currentImage = image.clone();
    displayRawImage(m_currentImage);
    setMonitorPage(0);

    ui->label_status_image->setText(
        tr("Image: %1 × %2 px").arg(image.cols).arg(image.rows));

    // Enable matching now that we have an image (and at least one group).
    ui->btn_run_match->setEnabled(ui->comboBox_pattern_group->count() > 0);

    setState(State::Idle, tr("Image ready."));
}

// ── Property browser change handlers ─────────────────────────────────────────

void LocalizationPatternsWidget::onPatternConfigModified() {
    // Pattern-level edits — adapter has already written to m_workingPatternCfg.
    // Commit through the manager so the live MatchPattern picks up the change.
    if (!m_patternManager || !m_boundPattern) return;
    if (m_selectedGroupIndex < 0)              return;

    auto group = m_patternManager->findGroupByNumber(m_selectedGroupIndex);
    if (!group) return;

    const QString groupName  = QString::fromStdWString(group->name());
    const QString patternKey = QString::fromStdWString(m_boundPattern->name());

    auto r = m_patternManager->setPatternConfig(
        groupName, patternKey, m_workingPatternCfg);
    if (!r) {
        QMessageBox::warning(this, tr("Update failed"), resultMessage(r));

        // Roll the working copy back to the live (unchanged) config and
        // refresh the adapter so the browser stops showing the rejected
        // edit.  The adapter blocks manager signals internally.
        m_workingPatternCfg = m_boundPattern->config();
        m_configAdapter->refresh();
        return;
    }

    // Re-resolve in case the rename/renumber changed identity.  The tree
    // row and pattern thumb are refreshed by the patternChanged listener.
    m_boundPattern = group->findPatternByName(
        m_workingPatternCfg.m_patternName);
}

void LocalizationPatternsWidget::onPropertyValueChanged(QtProperty *property,
                                                        const QVariant &value) {
    // Only handle group-level edits here.  Pattern-level edits flow through
    // MatchConfigPropertyAdapter, which emits configModified → handled by
    // onPatternConfigModified().
    if (!m_groupPropKeys.contains(property))         return;
    if (!m_patternManager || !m_boundMatchGroup)     return;

    const QString key = m_groupPropKeys.value(property);
    if (key.isEmpty()) return;

    if (!PropSpecHelper::dispatch(kMatchGroupSpecs, key, value, m_workingGroupConfig))
        return;

    // Snapshot the current name — dispatch may have already written a new
    // name into m_workingGroupConfig, but the live MatchGroup still carries
    // the old name, which is the lookup key for setGroupConfig.
    const QString oldName = QString::fromStdWString(m_boundMatchGroup->name());

    auto r = m_patternManager->setGroupConfig(oldName, m_workingGroupConfig);
    if (!r) {
        QMessageBox::warning(this, tr("Update failed"), resultMessage(r));

        // Roll the working copy back to the live (unchanged) state and
        // refresh the property browser so it stops showing the rejected
        // edit.  PropSpecHelper::refresh blocks manager signals internally.
        m_workingGroupConfig = m_boundMatchGroup->config();
        PropSpecHelper::refresh(m_variantManager, kMatchGroupSpecs,
                                m_workingGroupConfig, m_groupProps);
        return;
    }

    // Success.  Tree row, combo, and m_selectedGroupIndex are refreshed by
    // the PatternGroupManager::groupChanged listener (wireManagerSignals).
}

// ── Matching commission ─────────────────────────────────────────────

void LocalizationPatternsWidget::onMatchingCommissionDone(mtc::MatchResult result) {
    applyKpis(result);
    populateResultTable(result);     // ← fill the new result-table widget
    displayResultImage(result.Image);
    setMonitorPage(1);

    if (!result.isFoundMatchObject) {
        setState(State::Warning, tr("No match found."));
    } else if (result.isAreaLessThanLimits) {
        setState(State::Warning,
                 tr("Found %1 — material area below limit.")
                     .arg(result.totalPossiblePicking));
    } else {
        setState(State::Success,
                 tr("Found %1 object(s) in %2 ms")
                     .arg(result.totalPossiblePicking)
                     .arg(result.ExecutionTime, 0, 'f', 1));
    }
}

// ── Status / KPIs ────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::setState(State state, const QString &message) {
    static const QHash<State, QString> kLabel = {
        { State::Idle,    QStringLiteral("● IDLE")    },
        { State::Busy,    QStringLiteral("● BUSY")    },
        { State::Success, QStringLiteral("● OK")      },
        { State::Warning, QStringLiteral("● WARNING") },
        { State::Error,   QStringLiteral("● ERROR")   },
    };
    static const QHash<State, QString> kRole = {
        { State::Idle,    QStringLiteral("idle")    },
        { State::Busy,    QStringLiteral("busy")    },
        { State::Success, QStringLiteral("success") },
        { State::Warning, QStringLiteral("warning") },
        { State::Error,   QStringLiteral("error")   },
    };

    ui->label_state_pill->setText(kLabel.value(state));
    ui->label_state_pill->setProperty("pillState", kRole.value(state, QStringLiteral("idle")));
    ui->label_state_pill->style()->unpolish(ui->label_state_pill);
    ui->label_state_pill->style()->polish(ui->label_state_pill);
    ui->label_state_pill->update();

    ui->label_status_text->setText(message.isEmpty() ? tr("Ready") : message);
}

void LocalizationPatternsWidget::updateGroupsCount() {
    if (!m_patternManager) {
        ui->label_status_groups->setText(tr("Groups: 0  ·  Patterns: 0"));
        return;
    }
    int totalPatterns = 0;
    for (auto g : m_patternManager->groups())
        totalPatterns += g->patternCount();
    ui->label_status_groups->setText(
        tr("Groups: %1  ·  Patterns: %2")
            .arg(m_patternManager->groupCount()).arg(totalPatterns));
}

void LocalizationPatternsWidget::resetKpis() {
    ui->label_match_result_num->setText("—");
    ui->label_match_execution_time->setText("—");
    ui->label_match_lessthanlimit->setText("—");
    clearResultTable();
}

void LocalizationPatternsWidget::applyKpis(const mtc::MatchResult &result) {
    ui->label_match_total_objects->setText(
        QString::number(result.Objects.size()));
    ui->label_match_result_num->setText(
        QString::number(result.totalPossiblePicking));
    ui->label_match_execution_time->setText(
        tr("%1 ms").arg(result.ExecutionTime, 0, 'f', 1));
    ui->label_match_lessthanlimit ->setText(
        result.isAreaLessThanLimits ? tr("YES") : tr("NO"));
}

// ── Image display ────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::displayRawImage(const cv::Mat &image) {
    cv::Mat img = image;
    ui->imageView_Raw->loadImageOpenCv(img, true);
}

void LocalizationPatternsWidget::displayResultImage(const cv::Mat &image) {
    cv::Mat img = image;
    if (img.empty()) return;
    ui->imageView_Result->loadImageOpenCv(img, true);
}

void LocalizationPatternsWidget::setMonitorPage(int page) {
    ui->stackedWidget_ImageView->setCurrentIndex(page);
    ui->btn_view_raw   ->setChecked(page == 0);
    ui->btn_view_result->setChecked(page == 1);
}

// ── Matching test ────────────────────────────────────────────────────────────

bool LocalizationPatternsWidget::runMatchingTest(mtc::MatchResult &outResult) {
    if (!m_patternManager) return false;
    if (m_selectedGroupIndex < 0) return false;
    if (m_currentImage.empty()) return false;

    auto group = m_patternManager->findGroupByNumber(m_selectedGroupIndex);
    if (!group || group->isEmpty()) {
        setState(State::Warning, tr("Selected group has no patterns."));
        return false;
    }

    m_localizeTask->startCommissionMatching(group, m_currentImage);
    connect(m_localizeTask, &vc::model::TaskLocalization::commissionMatchingFinished,
            this, &LocalizationPatternsWidget::onMatchingCommissionDone, Qt::SingleShotConnection);

    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Result table (Pane 1 bottom)
//
//  Mirrors the design handoff `MonitorPane → Result Table`.  Columns:
//      #  |  Pat #  |  Pattern Name  |  Score  |  X  |  Y  |  Angle  |  OK
//
//  Inserted into pane_monitor's vertical layout right after frame_kpi so it
//  sits at the bottom of the monitor area.  Score is colour-coded:
//      ≥ 0.85 → green   ≥ 0.70 → orange   else → red
//
// -> disasbled OK/NG Row
// -> considering using OK/NG Row for dectected object under thresh score
// ─────────────────────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::buildResultTable() {
    if (!ui->pane_monitor) return;
    auto *layoutMon = qobject_cast<QVBoxLayout*>(ui->pane_monitor->layout());
    if (!layoutMon) return;

    // Section header
    auto *hd = new QLabel(tr("MATCH RESULTS"));
    hd->setObjectName(QStringLiteral("resultSectionHeader"));
    hd->setMinimumHeight(22);
    layoutMon->addWidget(hd);

    // Table
    m_resultTable = new QTableWidget(0, 7, ui->pane_monitor);
    m_resultTable->setObjectName(QStringLiteral("patternResultTable"));
    QStringList headers = {tr("#"), tr("Pat #"), tr("Pattern Name"),
                            tr("Score"), tr("X"), tr("Y"), tr("Angle") /*, tr("OK")*/ };
    m_resultTable->setHorizontalHeaderLabels(headers);
    m_resultTable->verticalHeader()->setVisible(false);
    m_resultTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_resultTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_resultTable->setSelectionMode(QAbstractItemView::SingleSelection);
    m_resultTable->setAlternatingRowColors(false);
    m_resultTable->setShowGrid(false);
    m_resultTable->setMinimumHeight(120);
    m_resultTable->setMaximumHeight(220);
    m_resultTable->horizontalHeader()->setStretchLastSection(false);
    m_resultTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    // Reasonable default column widths
    const int widths[8] = { 36, 48, 0 /* stretch */, 70, 70, 70, 60 /*, 50*/ };
    for (int i = 0; i < 7; ++i)
        if (widths[i] > 0) m_resultTable->setColumnWidth(i, widths[i]);

    // Styled via QSS by objectName "patternResultTable" in per-form QSS file.

    layoutMon->addWidget(m_resultTable);
}

void LocalizationPatternsWidget::clearResultTable() {
    if (m_resultTable) m_resultTable->setRowCount(0);
}

void LocalizationPatternsWidget::populateResultTable(const mtc::MatchResult &result) {
    if (!m_resultTable) return;

    m_resultTable->setRowCount(0);
    if (result.Objects.empty()) return;

    auto cell = [](const QString &text) {
        auto *it = new QTableWidgetItem(text);
        it->setTextAlignment(Qt::AlignCenter);
        return it;
    };
    auto colored = [](const QString &text, const QString &hex, bool bold = false) {
        auto *it = new QTableWidgetItem(text);
        it->setForeground(QBrush(QColor(hex)));
        it->setTextAlignment(Qt::AlignCenter);
        QFont f = it->font(); f.setBold(bold); it->setFont(f);
        return it;
    };

    int row = 0;
    for (const auto &obj : result.Objects) {
        m_resultTable->insertRow(row);

        const double score   = obj.matched_Score;
        // const QString scoreClr = score >= 0.85 ? QString(ptn::OK)
        //                         : score >= 0.70 ? QString(ptn::WARN)
        //                         :                 QString(ptn::ERR);
        const QString scoreClr = obj.hasCollision() ? QString(ptn::WARN) : QString(ptn::OK);

        // const bool ok = score >= 0.70;   // simple OK/NG threshold

        m_resultTable->setItem(row, 0, cell(QString::number(row + 1)));
        m_resultTable->setItem(row, 1, colored(QString("#%1").arg(obj.pattern_index),
                                                 ptn::OUTPUT, true));
        m_resultTable->setItem(row, 2, cell(QString::fromStdWString(obj.pattern_name)));
        m_resultTable->setItem(row, 3, colored(QString::number(score, 'f', 3),
                                                 scoreClr, true));
        m_resultTable->setItem(row, 4, cell(QString::number(obj.point_Center.x, 'f', 1)));
        m_resultTable->setItem(row, 5, cell(QString::number(obj.point_Center.y, 'f', 1)));
        m_resultTable->setItem(row, 6, cell(QString::number(obj.matched_Angle, 'f', 1) + "°"));
        // m_resultTable->setItem(row, 7, colored(ok ? "OK" : "NG",
        //                                          ok ? ptn::OK : ptn::ERR, true));
        ++row;
    }
}
