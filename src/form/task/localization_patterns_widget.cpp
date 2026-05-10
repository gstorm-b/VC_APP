#include "localization_patterns_widget.h"
#include "ui_localization_patterns_widget.h"

#include "form/pattern/pattern_manager_dialog.h"
#include "form/pattern/pattern_theme.h"
#include "form/pattern/add_pattern_wizard.h"
#include "form/pattern/edit_pattern_wizard.h"
#include "form/pattern/pattern_setting_panel.h"
#include "widgets/qtpropertybrowser/qtvariantproperty.h"

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
#include <QLabel>
#include <QStackedWidget>
#include <QFrame>
#include <QSplitter>
#include <QHash>
#include <QPair>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

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
    delete ui;
}

void LocalizationPatternsWidget::loadConfigToTask()   {}
void LocalizationPatternsWidget::loadConfigToWidget() {}

// ── initWidget ───────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::initWidget() {
    // ── Theme: apply dark palette tokens from design handoff ────────────────
    // Tokens defined in `form/pattern/pattern_theme.h`, mirroring
    // `ui_scratch/design_handoff_full_project/README.md`.
    setStyleSheet(QString(
        // ── Root ────────────────────────────────────────────────────────────
        "QWidget#LocalizationPatternsWidget { background: %1; color: %2; "
        "  font-family: 'Segoe UI'; }"
        "QSplitter::handle { background: %3; }"
        "QScrollBar:vertical { background: %1; width: 8px; }"
        "QScrollBar::handle:vertical { background: %4; border-radius: 3px; min-height: 20px; }"

        // ── Override .ui-defined frames whose hard-coded #1f2733 palette
        //     no longer matches the design tokens ──────────────────────────
        "QFrame#frame_view_header {"
        "  background: %5; border-radius: 0; border-bottom: 1px solid %3; }"
        "QFrame#frame_view_header QLabel {"
        "  color: %2; font: 700 9.5pt 'Segoe UI'; }"
        "QFrame#frame_view_header QPushButton {"
        "  color: %6; background: transparent;"
        "  border: 1px solid transparent; padding: 4px 14px; }"
        "QFrame#frame_view_header QPushButton:checked {"
        "  color: white; background: %7; border-bottom: 2px solid %7; }"
        "QFrame#frame_view_header QPushButton:hover:!checked { background: %8; }"

        "QFrame#frame_kpi { background: %5; border-radius: 0; "
        "  border-top: 1px solid %3; border-bottom: 1px solid %3; }"
        "QFrame#frame_kpi QLabel {"
        "  color: %6; font: 700 8.5pt 'Segoe UI'; letter-spacing: 1px; }"
        "QFrame#frame_kpi QLabel[role=\"axis\"] {"
        "  color: %7; font: 700 9pt 'JetBrains Mono'; }"
        "QFrame#frame_kpi QLabel[role=\"axis-val\"] {"
        "  color: %2; font: 700 11pt 'JetBrains Mono'; }"
        "QFrame#frame_kpi QLabel[role=\"kpi-value\"] {"
        "  color: %2; font: 800 12pt 'JetBrains Mono'; }"
        "QFrame#frame_kpi QLabel[role=\"kpi-label\"] {"
        "  color: %6; font: 700 8pt 'Segoe UI'; letter-spacing: 1px; }"

        "QFrame#frame_status { background: %5; border-top: 1px solid %3; }"
        "QFrame#frame_status QLabel { color: %4; font: 9pt 'Segoe UI'; }"

        // ── Pattern library tree itself ────────────────────────────────────
        "PatternTreeWidget { background: %1; color: %2; "
        "  border: none; outline: none; }"
        "PatternTreeWidget::item { background: transparent; }"
        "PatternTreeWidget::item:hover { background: %8; }"
        "PatternTreeWidget::item:selected { "
        "  background: rgba(43,140,232,28); color: %2; }"
    ).arg(ptn::BG,    /* %1 */ ptn::TXT,    /* %2 */ ptn::BD,    /* %3 */
           ptn::TXT2, /* %4 */ ptn::SURF,   /* %5 */ ptn::TXT3,  /* %6 */
           ptn::ACC,  /* %7 */ ptn::SURF2   /* %8 */));

    // Construct the QtPropertyBrowser objects but do NOT embed them anywhere —
    // PatternSettingPanel (see below) is now the canonical configuration UI on
    // the right pane.  We still need m_variantManager / m_variantEditor alive
    // because m_configAdapter is wired to them and rebuildPropertyBrowser()
    // dereferences both.  Passing nullptr leaves the browser as a hidden
    // orphan child of `this`; it costs nothing and keeps the legacy code path
    // crash-free.
    initPropertyBrowser(nullptr);

    // splitter ratios: monitor | tree+thumb (library pane) | inspector
    ui->splitter_main->setStretchFactor(0, 6);
    ui->splitter_main->setStretchFactor(1, 3);   // wider — now holds thumb too
    ui->splitter_main->setStretchFactor(2, 3);

    // ── Restructure: thumbnail panel goes below the tree, full configuration
    //                  panel goes into the right inspector pane ─────────────
    //
    // Pane 2 (library) layout — only thumbnail below the tree:
    //   ┌─ PATTERN LIBRARY ────────────────┐
    //   │ <PatternTreeWidget>              │  ← top half
    //   ├─ separator ──────────────────────┤
    //   │ <wg_thumb_container>             │  ← bottom: only thumbnail
    //   │   - "SELECTED PATTERN" header    │
    //   │   - QGraphicsView thumbnail      │
    //   │   - 1-line caption               │
    //   └──────────────────────────────────┘
    //
    // Pane 3 (inspector) layout — full PatternSettingPanel:
    //   ┌─ PATTERN CONFIGURATION ──────────┐
    //   │ <PatternSettingPanel>            │  ← title + LEARNED + buttons +
    //   │   (inline thumbnail HIDDEN —     │     match/edge/group rows
    //   │    we display it under tree)     │
    //   └──────────────────────────────────┘
    //
    // The QtPropertyBrowser is no longer displayed — we keep the adapter
    // wiring for backward compat but PatternSettingPanel is the sole UI.

    // Step A — relocate wg_thumb_container (defined inside splitter_property
    //          in the .ui) into the library pane, under the tree.
    if (ui->wg_thumb_container) {
        ui->wg_thumb_container->setParent(this);   // detach from inspector splitter
        ui->wg_thumb_container->show();            // make sure it is visible now
    }

    auto *libraryVSplitter = new QSplitter(Qt::Vertical, ui->pane_library);
    libraryVSplitter->setHandleWidth(2);
    libraryVSplitter->setChildrenCollapsible(false);
    libraryVSplitter->setStyleSheet(QString(
        "QSplitter::handle { background: %1; }"
    ).arg(ptn::BD));

    if (auto *tree = ui->treeWidget_patternEditor) {
        libraryVSplitter->addWidget(tree);
    }
    if (ui->wg_thumb_container) {
        libraryVSplitter->addWidget(ui->wg_thumb_container);
    }
    libraryVSplitter->setStretchFactor(0, 60);     // tree dominates
    libraryVSplitter->setStretchFactor(1, 40);     // thumb gets less

    if (ui->libraryLayout) {
        ui->libraryLayout->addWidget(libraryVSplitter, 1);
    }

    // Step B — install PatternSettingPanel inside ui->wg_property_browser
    //          (the empty placeholder host inside the right inspector pane).
    m_settingPanel = new PatternSettingPanel(this);
    m_settingPanel->setShowInlineThumbnail(false);   // dedicated thumb is below tree

    if (ui->wg_property_browser) {
        // initPropertyBrowser(nullptr) means no layout was created on the
        // placeholder — install one ourselves to host the panel.
        auto *propLay = ui->wg_property_browser->layout();
        if (!propLay) {
            propLay = new QVBoxLayout(ui->wg_property_browser);
            propLay->setContentsMargins(0, 0, 0, 0);
            propLay->setSpacing(0);
        }
        propLay->addWidget(m_settingPanel);
    }

    // Refresh the right-pane section title so it matches what the panel shows.
    if (ui->label_property_title) {
        ui->label_property_title->setText(tr("  PATTERN CONFIGURATION"));
    }

    // Wire setting-panel signals to host actions.
    connect(m_settingPanel, &PatternSettingPanel::learnRequested,
            this, &LocalizationPatternsWidget::onTriggerCameraClicked);
    connect(m_settingPanel, &PatternSettingPanel::openImageRequested,
            this, &LocalizationPatternsWidget::onChooseImageClicked);
    connect(m_settingPanel, &PatternSettingPanel::patternFieldChanged,
            this, &LocalizationPatternsWidget::onSettingPanelPatternFieldChanged);
    connect(m_settingPanel, &PatternSettingPanel::groupFieldChanged,
            this, &LocalizationPatternsWidget::onSettingPanelGroupFieldChanged);

    // splitter_property now hosts only the property container (thumb moved
    // out).  Let the property pane fill the available space.
    if (ui->splitter_property) {
        ui->splitter_property->setStretchFactor(0, 0);
        ui->splitter_property->setStretchFactor(1, 1);
    }

    // ── Pattern thumbnail scene — same QGraphicsView (now under the tree) ──
    m_thumbScene  = new QGraphicsScene(this);
    m_thumbPixmap = m_thumbScene->addPixmap(QPixmap{});
    if (ui->imageView_PatternThumb) {
        ui->imageView_PatternThumb->setScene(m_thumbScene);
        ui->imageView_PatternThumb->setRenderHint(QPainter::SmoothPixmapTransform);
    }

    // Theme the section labels (PATTERN LIBRARY, SELECTED PATTERN, ...)
    const QString sectionQss = ptn::sectionHeaderStyle();
    if (ui->label_library_title)    ui->label_library_title->setStyleSheet(sectionQss);
    if (ui->label_inspector_title)  ui->label_inspector_title->setStyleSheet(sectionQss);
    if (ui->label_property_title)   ui->label_property_title->setStyleSheet(sectionQss);
    if (ui->label_pattern_caption) {
        ui->label_pattern_caption->setStyleSheet(QString(
            "color: %1; font: 9pt 'Segoe UI'; padding: 2px;"
        ).arg(ptn::TXT3));
    }

    // Override the toolbar's hard-coded blue palette with design tokens.
    // The .ui file uses an older greyish-blue (#2b3340 etc.) — we replace
    // it here without rewriting the XML so the design+code stay in sync.
    if (ui->frame_toolbar) {
        ui->frame_toolbar->setObjectName("frame_toolbar");   // already set in .ui
        ui->frame_toolbar->setStyleSheet(QString(
            "QFrame#frame_toolbar { background: %1; border-bottom: 1px solid %2; }"
            "QFrame#frame_toolbar QToolButton {"
            "  color: %3; background: transparent;"
            "  border: 1px solid transparent; padding: 4px 12px; border-radius: 4px;"
            "  font: 600 10pt 'Segoe UI'; }"
            "QFrame#frame_toolbar QToolButton:hover { background: %4; border: 1px solid %5; color: %6; }"
            "QFrame#frame_toolbar QToolButton:pressed { background: %1; }"
            "QFrame#frame_toolbar QToolButton#btn_run_match {"
            "  background: %7; border: 1px solid %7; color: white; }"
            "QFrame#frame_toolbar QToolButton#btn_run_match:hover {"
            "  background: %8; }"
            "QFrame#frame_toolbar QToolButton#btn_run_match:disabled {"
            "  background: %4; color: %9; border-color: %2; }"
            "QFrame#frame_toolbar QLabel { color: %3; font: 10pt 'Segoe UI'; }"
            "QFrame#frame_toolbar QFrame[role=\"separator\"] { background: %2; max-width: 1px; min-width: 1px; }"
            "QFrame#frame_toolbar QComboBox { background: %1; color: %6; "
            "  border: 1px solid %5; border-radius: 4px; padding: 3px 8px; }"
            "QFrame#frame_toolbar QCheckBox { color: %3; font: 10pt 'Segoe UI'; }"
        ).arg(ptn::HD,    /* %1 */ ptn::BD,    /* %2 */ ptn::TXT2,  /* %3 */
               ptn::SURF2, /* %4 */ ptn::BD2,  /* %5 */ ptn::TXT,    /* %6 */
               ptn::OK,    /* %7 */ "#3ad88f", /* %8 */ ptn::TXT4    /* %9 */));
    }

    // Camera-image dialog (legacy capture flow used by the new wizard)
    m_addPatternImageDialog = new AddPatternImageDialog(this);
    connect(m_addPatternImageDialog, &AddPatternImageDialog::requestImage,
            this, &LocalizationPatternsWidget::requestCameraImage);

    // Bind to the task
    if (m_task) {
        m_localizeTask   = static_cast<vc::model::TaskLocalization*>(m_task.get());
        m_config         = m_localizeTask->taskLocalizeConfig();
        m_patternManager = m_localizeTask->patternManager();
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
        // Mirror pre-existing patterns so the tree shows complete state.
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
    connect(m_patternManager, &mtc::PatternGroupManager::groupChanged,
            this, [this] (std::shared_ptr<mtc::MatchGroup> group, const QString &) {
        if (!group) return;
        // Refresh under-tree setting panel if this group is currently shown.
        if (m_settingPanel && m_selectedGroupIndex == group->number()) {
            mtc::MatchPattern *pat = nullptr;
            if (m_selectedPatternIndex >= 0)
                pat = group->findPatternByNumber(m_selectedPatternIndex);
            m_settingPanel->setSelection(group.get(), pat);
        }
        rebuildGroupCombo();
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
            if (m_settingPanel) m_settingPanel->setSelection(group, nullptr);
        }
        updateGroupsCount();
    });

    // ── Pattern changed (config edit / rename) ─────────────────────────────
    connect(m_patternManager, &mtc::PatternGroupManager::patternChanged,
            this, [this] (mtc::MatchGroup *group, mtc::MatchPattern *pattern,
                          const QString &) {
        if (!group || !pattern) return;
        if (m_settingPanel
            && m_selectedGroupIndex   == group->number()
            && m_selectedPatternIndex == pattern->number()) {
            m_settingPanel->setSelection(group, pattern);
        }
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

void LocalizationPatternsWidget::buildGroupProperties() {
    m_grpGroupProps = m_variantManager->addProperty(
        QtVariantPropertyManager::groupTypeId(), tr("Group Settings"));

    auto add = [&](int type, const QString &label,
                   const QVariant &min, const QVariant &max,
                   const QVariant &step, const QVariant &value) -> QtVariantProperty* {
        auto *p = m_variantManager->addProperty(type, label);
        if (min.isValid())  m_variantManager->setAttribute(p, "minimum",    min);
        if (max.isValid())  m_variantManager->setAttribute(p, "maximum",    max);
        if (step.isValid()) m_variantManager->setAttribute(p, "singleStep", step);
        if (type == QVariant::Double)
            m_variantManager->setAttribute(p, "decimals", 3);
        p->setValue(value);
        m_grpGroupProps->addSubProperty(p);
        m_groupPropSet.insert(p);
        return p;
    };

    // Pull defaults from currently selected group, if any.
    mtc::MatchGroupConfig cfg;
    if (m_patternManager && m_selectedGroupIndex >= 0) {
        if (auto g = m_patternManager->findGroupByNumber(m_selectedGroupIndex))
            cfg = g->config();
    }

    m_propGroupRatio = add(QVariant::Double, tr("Low Workpiece Ratio"),
                           0.0, 100.0, 0.1, cfg.m_lowWorkpieceRatio);
    m_propPickBoxW   = add(QVariant::Double, tr("Picking Box Width"),
                           0.0, 10000.0, 1.0, cfg.m_pickingBoxSize.width);
    m_propPickBoxH   = add(QVariant::Double, tr("Picking Box Height"),
                           0.0, 10000.0, 1.0, cfg.m_pickingBoxSize.height);
    m_propPickBoxDist = add(QVariant::Double, tr("Picking Box Distance"),
                            0.0, 10000.0, 1.0, cfg.m_pickingBoxDistance);
    m_propPickBoxAngle = add(QVariant::Double, tr("Picking Box Angle (°)"),
                             -360.0, 360.0, 1.0, cfg.m_pickingBoxAngle);
}

void LocalizationPatternsWidget::rebuildPropertyBrowser() {
    if (!m_variantManager || !m_variantEditor || !m_configAdapter) return;

    // ── Order matters here ────────────────────────────────────────────────
    // 1. Clear the BROWSER first.  This drops every QtBrowserItem reference
    //    while the underlying QtProperty objects are still alive — so the
    //    browser releases its observer connections cleanly, no danglers.
    //
    // 2. Then tear down the adapter.  bind(nullptr) calls m_mgr->clear()
    //    which destroys every QtProperty in the manager.  Because the
    //    browser is already empty it has nothing to react to.
    //
    // 3. Reset our own pointer bookkeeping (the QtProperty objects they
    //    pointed to were just destroyed by step 2).
    //
    // 4. Re-bind / rebuild and finally re-attach to the browser.
    //
    // Previously this method wrapped the whole sequence in a
    // QSignalBlocker(m_variantManager).  That blocked the manager's
    // `propertyDestroyed` signal as well — the browser never observed
    // destruction, kept a stale QtProperty* in its internal map, and the
    // very next `m_variantEditor->clear()` chased that dangling pointer
    // → crash on the second selection change.

    m_variantEditor->clear();
    m_configAdapter->bind(nullptr);

    m_groupPropSet.clear();
    m_grpGroupProps    = nullptr;
    m_propGroupRatio   = nullptr;
    m_propPickBoxW     = nullptr;
    m_propPickBoxH     = nullptr;
    m_propPickBoxDist  = nullptr;
    m_propPickBoxAngle = nullptr;

    if (m_boundPattern)
        m_configAdapter->bind(&m_workingPatternCfg);

    buildGroupProperties();

    if (m_grpGroupProps)
        m_variantEditor->addProperty(m_grpGroupProps);
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
    unbindPattern();
    updatePatternThumb(nullptr);

    // Push selection into the under-tree PatternSettingPanel
    if (m_settingPanel && m_patternManager) {
        auto group = m_patternManager->findGroupByNumber(groupIndex);
        m_settingPanel->setSelection(group.get(), nullptr);
    }
}

void LocalizationPatternsWidget::selectPattern(int groupIndex, int patternIndex) {
    m_selectedGroupIndex   = groupIndex;
    m_selectedPatternIndex = patternIndex;

    if (!m_patternManager) return;
    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) return;

    auto *pattern = group->findPatternByNumber(patternIndex);
    bindPatternToBrowser(pattern);
    updatePatternThumb(pattern);

    // Push selection into the under-tree PatternSettingPanel
    if (m_settingPanel) {
        m_settingPanel->setSelection(group.get(), pattern);
    }
}

void LocalizationPatternsWidget::clearSelection() {
    m_selectedGroupIndex   = -1;
    m_selectedPatternIndex = -1;
    unbindPattern();
    updatePatternThumb(nullptr);
    if (m_settingPanel) m_settingPanel->clearSelection();
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
    if (m_settingPanel) m_settingPanel->setPatternThumbnail(pm);
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
    // the legacy AddPatternImageDialog).
    connect(&wiz, &AddPatternWizard::requestCameraImage,
            this, &LocalizationPatternsWidget::requestCameraImage);

    const int wizResult = wiz.exec();
    m_activeAddWizard = nullptr;
    if (wizResult != QDialog::Accepted) return;

    cv::Mat patternImage = wiz.patternImage();
    if (patternImage.empty()) return;

    // Apply crop if user did not check "use original frame"
    if (!wiz.keepOriginal()) {
        const QRect r = wiz.cropRect();
        if (r.width() > 0 && r.height() > 0
            && r.x() >= 0 && r.y() >= 0
            && r.x() + r.width()  <= patternImage.cols
            && r.y() + r.height() <= patternImage.rows)
        {
            patternImage = patternImage(cv::Rect(r.x(), r.y(),
                                                 r.width(), r.height())).clone();
        }
    }

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
        const QList<MatchGroupConfig> &) {}

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
    setState(State::Busy, tr("Waiting for camera image…"));
    emit requestCameraImage();
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
        QMessageBox::warning(this, tr("Update failed"),
                             resultMessage(r));
        return;
    }

    // Re-resolve in case the rename/renumber changed identity.
    m_boundPattern = group->findPatternByName(
        m_workingPatternCfg.m_patternName);
    updatePatternThumb(m_boundPattern);
}

void LocalizationPatternsWidget::onPropertyValueChanged(QtProperty *property,
                                                        const QVariant &) {
    if (!m_groupPropSet.contains(property)) return;     // adapter handles its own
    if (!m_patternManager) return;
    if (m_selectedGroupIndex < 0) return;

    auto group = m_patternManager->findGroupByNumber(m_selectedGroupIndex);
    if (!group) return;

    mtc::MatchGroupConfig cfg = group->config();
    cfg.m_lowWorkpieceRatio   = m_propGroupRatio  ->value().toDouble();
    cfg.m_pickingBoxSize      = cv::Size2f(
                                   static_cast<float>(m_propPickBoxW->value().toDouble()),
                                   static_cast<float>(m_propPickBoxH->value().toDouble()));
    cfg.m_pickingBoxDistance  = m_propPickBoxDist ->value().toDouble();
    cfg.m_pickingBoxAngle     = m_propPickBoxAngle->value().toDouble();

    const QString groupName = QString::fromStdWString(group->name());
    auto r = m_patternManager->setGroupConfig(groupName, cfg);
    if (!r) {
        QMessageBox::warning(this, tr("Update failed"),
                             resultMessage(r));
    }
}

// ── Status / KPIs ────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::setState(State state, const QString &message) {
    static const QHash<State, QPair<QString, QString>> kPalette = {
        { State::Idle,    {"#d6dde6", "#4a5b73"} },
        { State::Busy,    {"#ffffff", "#2b6cd9"} },
        { State::Success, {"#ffffff", "#2c8b3f"} },
        { State::Warning, {"#1a212c", "#f0b400"} },
        { State::Error,   {"#ffffff", "#c0392b"} },
    };
    static const QHash<State, QString> kLabel = {
        { State::Idle,    QStringLiteral("● IDLE")    },
        { State::Busy,    QStringLiteral("● BUSY")    },
        { State::Success, QStringLiteral("● OK")      },
        { State::Warning, QStringLiteral("● WARNING") },
        { State::Error,   QStringLiteral("● ERROR")   },
    };

    const auto pair = kPalette.value(state, {"#d6dde6", "#4a5b73"});
    ui->label_state_pill->setText(kLabel.value(state));
    ui->label_state_pill->setStyleSheet(pillStyle(pair.first, pair.second));

    if (!message.isEmpty()) {
        ui->label_status_text->setText(message);
    } else {
        ui->label_status_text->setText(tr("Ready"));
    }
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
    ui->label_match_result_num   ->setText("—");
    ui->label_match_execution_time->setText("—");
    ui->label_match_lessthanlimit ->setText("—");
    ui->label_pickpos_x->setText("0.00");
    ui->label_pickpos_y->setText("0.00");
    ui->label_pickpos_z->setText("0.00");
    ui->label_pickpos_r->setText("0.00");
    clearResultTable();
}

void LocalizationPatternsWidget::applyKpis(const mtc::MatchResult &result) {
    ui->label_match_result_num   ->setText(
        QString::number(result.totalPossiblePicking));
    ui->label_match_execution_time->setText(
        tr("%1 ms").arg(result.ExecutionTime, 0, 'f', 1));
    ui->label_match_lessthanlimit ->setText(
        result.isAreaLessThanLimits ? tr("YES") : tr("NO"));

    if (!result.Objects.empty()) {
        const auto &top = result.Objects.front();
        ui->label_pickpos_x->setText(QString::number(top.point_Center.x, 'f', 2));
        ui->label_pickpos_y->setText(QString::number(top.point_Center.y, 'f', 2));
        ui->label_pickpos_z->setText("0.00");   // depth not estimated here
        ui->label_pickpos_r->setText(QString::number(top.point_angle,    'f', 2));
    } else {
        ui->label_pickpos_x->setText("0.00");
        ui->label_pickpos_y->setText("0.00");
        ui->label_pickpos_z->setText("0.00");
        ui->label_pickpos_r->setText("0.00");
    }
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

    // Build a fresh matcher and populate with copies of the group's patterns.
    auto matcher = std::make_unique<mtc::ImageMatcher>();
    auto *model  = matcher->getModel();

    for (const auto &patternPtr : group->patterns()) {
        if (!patternPtr) continue;

        mtc::MatchPatternConfig cfg = patternPtr->config();
        if (cfg.m_rawImage.empty()) continue;

        auto r = model->addPattern(cfg);
        if (!r) continue;

        auto last = model->lastPatternAccess();
        if (!last) continue;

        cv::Mat trainImg = cfg.m_rawImage.clone();
        if (!last->learnPattern(trainImg)) {
            qWarning() << "[LocalizationPatternsWidget] learnPattern failed for"
                       << QString::fromStdWString(cfg.m_patternName);
        }
    }

    if (model->isEmpty()) return false;

    matcher->setImageSource(m_currentImage.clone());
    matcher->matching(false, -1, false);

    outResult = matcher->match_result;
    return true;
}

// ── PatternSettingPanel field change handlers ──────────────────────────────
//
// The under-tree panel emits coarse "field changed" signals; we apply each
// change to the live MatchPattern / MatchGroup so:
//   - the property browser on the right stays in sync,
//   - the matcher uses fresh values on the next run,
//   - the manager fires its normal pattern/group change notifications.

void LocalizationPatternsWidget::onSettingPanelPatternFieldChanged(
        const QString &key, const QVariant &value) {
    if (!m_patternManager
        || m_selectedGroupIndex   < 0
        || m_selectedPatternIndex < 0) return;

    auto group = m_patternManager->findGroupByNumber(m_selectedGroupIndex);
    if (!group) return;
    auto *pat  = group->findPatternByNumber(m_selectedPatternIndex);
    if (!pat) return;

    mtc::MatchPatternConfig cfg = pat->config();
    bool ok = true;

    if      (key == "minScore")        cfg.m_minScore       = value.toDouble();
    else if (key == "angle")           cfg.m_angle          = value.toDouble();
    else if (key == "toleranceAngle")  cfg.m_toleranceAngle = value.toDouble();
    else if (key == "maxOverlap")      cfg.m_maxOverlap     = value.toDouble();
    else if (key == "pickX")           cfg.m_pickPosition.x = static_cast<float>(value.toInt());
    else if (key == "pickY")           cfg.m_pickPosition.y = static_cast<float>(value.toInt());
    else if (key.startsWith("edge.")) {
        if (auto *e = cfg.edgeConfig()) {
            const QString sub = key.mid(5);
            if      (sub == "threshLower")     e->threshLower           = value.toDouble();
            else if (sub == "threshUpper")     e->threshUpper           = value.toDouble();
            else if (sub == "kernelSize")      e->kernelSize            = value.toInt();
            else if (sub == "blurW")           e->blurWidth             = value.toInt();
            else if (sub == "blurH")           e->blurHeight            = value.toInt();
            else if (sub == "greediness")      e->greediness            = value.toDouble();
            else if (sub == "minReduceLength") e->minReduceLength       = value.toInt();
            else if (sub == "tSamples")        e->tSamples              = value.toInt();
            else if (sub == "invertBinary")    e->invertBinaryThreshold = value.toBool();
            else if (sub == "subPixel")        e->subPixelEstimation    = value.toBool();
            else if (sub == "stopAtLayer1")    e->stopAtLayer1          = value.toBool();
            else ok = false;
        } else {
            ok = false;
        }
    } else {
        ok = false;
    }

    if (!ok) return;
    pat->setConfig(cfg);

    // Mirror into the right-side property browser (if it's currently bound
    // to this pattern)
    if (m_boundPattern == pat) {
        m_workingPatternCfg = cfg;
    }
}

void LocalizationPatternsWidget::onSettingPanelGroupFieldChanged(
        const QString &key, const QVariant &value) {
    if (!m_patternManager || m_selectedGroupIndex < 0) return;

    auto group = m_patternManager->findGroupByNumber(m_selectedGroupIndex);
    if (!group) return;

    mtc::MatchGroupConfig cfg = group->config();
    bool ok = true;

    if      (key == "lowWorkpieceRatio") cfg.m_lowWorkpieceRatio  = value.toDouble();
    else if (key == "pickBoxW")          cfg.m_pickingBoxSize.width  = static_cast<float>(value.toDouble());
    else if (key == "pickBoxH")          cfg.m_pickingBoxSize.height = static_cast<float>(value.toDouble());
    else if (key == "pickBoxDist")       cfg.m_pickingBoxDistance = value.toDouble();
    else if (key == "pickBoxAngle")      cfg.m_pickingBoxAngle    = value.toDouble();
    else                                  ok = false;

    if (!ok) return;
    // MatchGroup::setConfig is private (only PatternGroupManager can call it
    // — it validates name / number uniqueness against siblings).  Route
    // through the manager which exposes setGroupConfig publicly.
    const QString groupName = QString::fromStdWString(group->name());
    m_patternManager->setGroupConfig(groupName, cfg);
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
// ─────────────────────────────────────────────────────────────────────────────

void LocalizationPatternsWidget::buildResultTable() {
    if (!ui->pane_monitor) return;
    auto *layoutMon = qobject_cast<QVBoxLayout*>(ui->pane_monitor->layout());
    if (!layoutMon) return;

    // Section header
    auto *hd = new QLabel(tr("MATCH RESULTS"));
    hd->setStyleSheet(QString(
        "background: %1; color: %2; "
        "font: 700 8pt 'Segoe UI'; letter-spacing: 1.2px; "
        "padding: 4px 10px; border-bottom: 1px solid %3; "
        "border-top: 1px solid %3;"
    ).arg(ptn::HD, ptn::TXT3, ptn::BD));
    hd->setMinimumHeight(22);
    layoutMon->addWidget(hd);

    // Table
    m_resultTable = new QTableWidget(0, 8, ui->pane_monitor);
    QStringList headers = {tr("#"), tr("Pat #"), tr("Pattern Name"),
                            tr("Score"), tr("X"), tr("Y"), tr("Angle"), tr("OK")};
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
    const int widths[8] = { 36, 48, 0 /* stretch */, 70, 70, 70, 60, 50 };
    for (int i = 0; i < 8; ++i)
        if (widths[i] > 0) m_resultTable->setColumnWidth(i, widths[i]);

    m_resultTable->setStyleSheet(QString(
        "QTableWidget { background: %1; color: %2; gridline-color: %3; "
        "  border: none; outline: none; "
        "  font: 9.5pt 'JetBrains Mono'; }"
        "QTableWidget::item { padding: 4px 8px; border-bottom: 1px solid %3; }"
        "QTableWidget::item:selected { "
        "  background: rgba(43,140,232,28); color: %2; }"
        "QHeaderView::section { background: %4; color: %5; "
        "  border: none; border-bottom: 1px solid %3; "
        "  padding: 4px 8px; font: 700 8pt 'Segoe UI'; "
        "  letter-spacing: 0.6px; text-transform: uppercase; }"
    ).arg(ptn::BG,    /* %1 */ ptn::TXT,  /* %2 */ ptn::BD,
           ptn::SURF, /* %4 */ ptn::TXT3 /* %5 */));

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
        const QString scoreClr = score >= 0.85 ? QString(ptn::OK)
                                : score >= 0.70 ? QString(ptn::WARN)
                                :                 QString(ptn::ERR);
        const bool ok = score >= 0.70;   // simple OK/NG threshold

        m_resultTable->setItem(row, 0, cell(QString::number(row + 1)));
        m_resultTable->setItem(row, 1, colored(QString("#%1").arg(obj.pattern_index),
                                                 ptn::OUTPUT, true));
        m_resultTable->setItem(row, 2, cell(QString::fromStdWString(obj.pattern_name)));
        m_resultTable->setItem(row, 3, colored(QString::number(score, 'f', 3),
                                                 scoreClr, true));
        m_resultTable->setItem(row, 4, cell(QString::number(obj.point_Center.x, 'f', 1)));
        m_resultTable->setItem(row, 5, cell(QString::number(obj.point_Center.y, 'f', 1)));
        m_resultTable->setItem(row, 6, cell(QString::number(obj.matched_Angle, 'f', 1) + "°"));
        m_resultTable->setItem(row, 7, colored(ok ? "OK" : "NG",
                                                 ok ? ptn::OK : ptn::ERR, true));
        ++row;
    }
}
