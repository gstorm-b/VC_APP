#include "localization_patterns_widget.h"
#include "ui_localization_patterns_widget.h"

#include "form/pattern/pattern_manager_dialog.h"
#include "widgets/qtpropertybrowser/qtvariantproperty.h"

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
    initPropertyBrowser(ui->wg_property_browser);

    // splitter ratios: monitor | tree | inspector
    ui->splitter_main->setStretchFactor(0, 6);
    ui->splitter_main->setStretchFactor(1, 2);
    ui->splitter_main->setStretchFactor(2, 3);

    // inspector internal split: thumbnail | properties
    ui->splitter_property->setStretchFactor(0, 3);
    ui->splitter_property->setStretchFactor(1, 7);

    // Pattern thumbnail scene
    m_thumbScene  = new QGraphicsScene(this);
    m_thumbPixmap = m_thumbScene->addPixmap(QPixmap{});
    ui->imageView_PatternThumb->setScene(m_thumbScene);
    ui->imageView_PatternThumb->setRenderHint(QPainter::SmoothPixmapTransform);
    ui->imageView_PatternThumb->setBackgroundBrush(QColor("#11161e"));

    // Camera-image dialog for "Add pattern".  Forward its requestImage()
    // through our own signal so the host wires camera capture in one place.
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
    wirePropertyBrowser();

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

    connect(m_patternManager, &mtc::PatternGroupManager::groupAdded,
            this, [this] (std::shared_ptr<mtc::MatchGroup> group) {
        MatchGroupConfig gr;
        gr.name   = QString::fromStdWString(group->name());
        gr.number = group->number();
        ui->treeWidget_patternEditor->addGroup(gr);
        rebuildGroupCombo();
        updateGroupsCount();
    });

    connect(m_patternManager, &mtc::PatternGroupManager::groupRemoved,
            this, [this] (const mtc::MatchGroupConfig &removed) {
        Q_UNUSED(removed);
        rebuildGroupCombo();
        updateGroupsCount();
    });

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

    connect(m_patternManager, &mtc::PatternGroupManager::patternRemoved,
            this, [this] (mtc::MatchGroup *, const mtc::MatchPatternConfig &) {
        updateGroupsCount();
    });
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
    QSignalBlocker blocker(m_variantManager);

    // 1. Tear down the adapter first.  bind(nullptr) calls destroy() which
    //    clears the manager — so doing this *before* we rebuild our own
    //    group properties guarantees no stale items survive.
    m_configAdapter->bind(nullptr);

    // 2. Clear browser items and our own bookkeeping.
    m_variantEditor->clear();
    m_groupPropSet.clear();
    m_grpGroupProps    = nullptr;
    m_propGroupRatio   = nullptr;
    m_propPickBoxW     = nullptr;
    m_propPickBoxH     = nullptr;
    m_propPickBoxDist  = nullptr;
    m_propPickBoxAngle = nullptr;

    // 3. Re-bind adapter to the current pattern (rebuilds its own props
    //    inside the manager).
    if (m_boundPattern)
        m_configAdapter->bind(&m_workingPatternCfg);

    // 4. Build group-level properties — these coexist with adapter props.
    buildGroupProperties();

    // 5. Add to browser in display order: group settings first, then
    //    pattern-specific common + algorithm groups.
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
}

void LocalizationPatternsWidget::clearSelection() {
    m_selectedGroupIndex   = -1;
    m_selectedPatternIndex = -1;
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

    int rc = m_addPatternImageDialog->showAddPatternDialog();
    if (rc != QDialog::Accepted) return;

    cv::Mat patternImage = m_addPatternImageDialog->getFinalImage();
    if (patternImage.empty()) return;

    QString title = tr("Input new Pattern information");
    AddPatternDialog dialog(title, this);
    if (dialog.exec() != QDialog::Accepted) return;

    const QString name  = dialog.getPatternName();
    const int     index = dialog.getPatternIndex();

    auto group = m_patternManager->findGroupByNumber(groupIndex);
    if (!group) {
        QMessageBox::warning(this, tr("Add pattern error"),
                             tr("Group not found."));
        return;
    }

    const QString groupName = QString::fromStdWString(group->name());

    if (group->containsPatternName(name.toStdWString())) {
        QMessageBox::warning(this, tr("Duplicated pattern name"),
                             tr("Pattern name %1 already exists in group %2.")
                                 .arg(name).arg(groupName));
        return;
    }
    if (group->containsPatternNumber(index)) {
        QMessageBox::warning(this, tr("Duplicated pattern index"),
                             tr("Pattern index %1 already exists in group %2.")
                                 .arg(index).arg(groupName));
        return;
    }

    mtc::MatchPatternConfig cfg;
    cfg.m_patternName  = name.toStdWString();
    cfg.m_patternIndex = index;
    cfg.m_rawImage     = patternImage.clone();

    auto r = m_patternManager->addPattern(groupName, cfg);
    if (!r) {
        QMessageBox::warning(this, tr("Add pattern failed"),
                             resultMessage(r));
    }
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

    // Route the image to whoever asked for it.  When the AddPatternImage
    // dialog is open it owns the capture flow, so feed it directly and
    // skip the main monitor update.
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
