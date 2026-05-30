#ifndef LOCALIZATION_PATTERNS_WIDGET_H
#define LOCALIZATION_PATTERNS_WIDGET_H

#include <QWidget>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QSet>

#include <opencv2/core.hpp>

#include "DockWidget.h"

#include "model/task_localization.h"
#include "form/task_widget.h"
#include "widgets/pattern_tree_widget.h"

#include "form/pattern/add_pattern_image_dialog.h"
#include "matching/match_config_property_adapter.h"
#include "matching/image_matcher.h"

namespace Ui {
class LocalizationPatternsWidget;
}

// ---------------------------------------------------------------------------
// LocalizationPatternsWidget
//
//  Three responsibilities:
//   1. Pattern management — add/remove/rename groups + patterns via the
//      PatternGroupManager held by the task.
//   2. Match configuration — bind the selected pattern's MatchPatternConfig
//      and the selected group's MatchGroupConfig to a property browser.
//   3. Matching test — run a matching pass on a test image and visualise
//      the result.  Image source is provided externally: connect
//      `requestCameraImage()` on the host side and feed back via
//      `setCameraImage()`.
// ---------------------------------------------------------------------------
class LocalizationPatternsWidget : public ITaskWidget {
    Q_OBJECT

public:
    explicit LocalizationPatternsWidget(std::shared_ptr<vc::model::ITask> task,
                                        ads::CDockWidget *dock = nullptr,
                                        QWidget *parent = nullptr);
    ~LocalizationPatternsWidget();

    void loadConfigToTask() override;
    void loadConfigToWidget() override;

signals:
    /// Emitted when the user clicks "Trigger Camera".  Host should capture
    /// an image from the active camera and feed it back via setCameraImage().
    void requestCameraImage(QString id);

public slots:
    /// External entry point for image input (camera, file, network …).
    void setCameraImage(const cv::Mat &image);

    /// Open the Edit Pattern Wizard for a specific (groupNumber, patternNumber).
    /// Returns true if the user accepted and the pattern was updated.
    /// Invoke from a host-supplied UI control (e.g. a "Edit" toolbar button
    /// or a keyboard shortcut), or from the pattern tree's edit-icon click.
    bool editPattern(int groupNumber, int patternNumber);

    /// Open the Edit Pattern Wizard for the currently-selected pattern,
    /// or do nothing if no pattern is selected.
    void editSelectedPattern();

private slots:
    // ── Pattern tree handlers ────────────────────────────────────────────
    void onTreeGroupClicked   (int groupIndex, const MatchGroupConfig &cfg);
    void onTreePatternClicked (int groupIndex, int patternIndex,
                               const MatchPatternConfig &cfg);

    void onTreeAddGroupRequested();
    void onTreeAddPatternRequested  (int groupIndex);
    void onTreeDeleteGroupRequested (int groupIndex);
    void onTreeDeletePatternRequested(int groupIndex, int patternIndex);

    void onTreeGroupsChanged (const QList<MatchGroupConfig> &groups);
    void onTreeGroupChanged  (int groupIndex, const MatchGroupConfig &cfg);
    void onTreePatternChanged(int groupIndex, int patternIndex,
                              const MatchPatternConfig &cfg);

    // ── Toolbar actions ──────────────────────────────────────────────────
    void onTriggerCameraClicked();
    void onChooseImageClicked();
    void onSetWorkspaceClicked();
    void onRunMatchingClicked();
    void onActiveGroupChanged(int comboIndex);

    // ── View tab toggle (Raw / Result) ───────────────────────────────────
    void onShowRawView();
    void onShowResultView();

    // ── Property browser ────────────────────────────────────────────────
    void onPatternConfigModified();
    void onPropertyValueChanged(QtProperty *property, const QVariant &value);

    // ── Matching commission ─────────────────────────────────────────────
    void onMatchingCommissionDone(mtc::MatchResult result);

private:
    // ── Build / wire-up ─────────────────────────────────────────────────
    void initWidget();
    void wireToolbar();
    void wireTree();
    void wireManagerSignals();
    void wirePropertyBrowser();
    void rebuildTreeFromManager();   // pull full library state into the tree
    void buildResultTable();         // build & insert result-table widget
    void populateResultTable(const mtc::MatchResult &result);
    void clearResultTable();

    // ── Property browser construction ───────────────────────────────────
    void buildGroupProperties();      // group-level (picking box etc.)
    void rebuildPropertyBrowser();    // re-add group + adapter properties
    void bindPatternToBrowser(mtc::MatchPattern *pattern);
    void unbindPattern();

    // ── Selection state ─────────────────────────────────────────────────
    void selectGroup(int groupIndex);
    void selectPattern(int groupIndex, int patternIndex);
    void clearSelection();
    void updatePatternThumb(mtc::MatchPattern *pattern);

    // ── Camera combo ────────────────────────────────────────────────────
    void rebuildCameraCombo();

    // ── Group combo ─────────────────────────────────────────────────────
    void rebuildGroupCombo();

    // ── Status / KPI / state pill ───────────────────────────────────────
    enum class State { Idle, Busy, Success, Warning, Error };
    void  setState(State state, const QString &message = {});
    void  updateGroupsCount();
    void  resetKpis();
    void  applyKpis(const mtc::MatchResult &result);

    // ── Image display ───────────────────────────────────────────────────
    void displayRawImage(const cv::Mat &image);
    void displayResultImage(const cv::Mat &image);
    void setMonitorPage(int page);   // 0 = raw, 1 = result

    // ── Matching test runner ────────────────────────────────────────────
    bool runMatchingTest(mtc::MatchResult &outResult);

    // ── Helpers ─────────────────────────────────────────────────────────
    static QPixmap matToPixmap(const cv::Mat &mat);

private:
    Ui::LocalizationPatternsWidget *ui;

    vc::model::TaskLocalization     *m_localizeTask{nullptr};
    vc::model::TaskLocalizeConfig    m_config;

    mtc::PatternGroupManager        *m_patternManager{nullptr};
    mtc::MatchConfigPropertyAdapter *m_configAdapter {nullptr};

    AddPatternImageDialog           *m_addPatternImageDialog{nullptr};

    // Active Add Pattern wizard (non-null only while wizard is open).
    // Lets setCameraImage() forward captures into the wizard preview.
    class AddPatternWizard           *m_activeAddWizard{nullptr};

    // Result table — sits below the KPI strip in Pane 1, inside a vertical
    // splitter so the user can resize image vs. table.  Populated after each
    // matching run from `mtc::MatchResult`.  Mirrors the Result Table from
    // `PatternManager.jsx → MonitorPane`.
    class QTableWidget               *m_resultTable{nullptr};

    // ── Selection state (-1 = none) ─────────────────────────────────────
    int m_selectedGroupIndex  {-1};   // group *number*, not list index
    int m_selectedPatternIndex{-1};

    // ── Working pattern config — adapter writes through this object,
    //    and we commit it back to the manager on edits. ─────────────────
    mtc::MatchGroupConfig           m_workingGroupConfig;
    mtc::MatchGroup                 *m_boundMatchGroup{nullptr};
    mtc::MatchPatternConfig          m_workingPatternCfg;
    mtc::MatchPattern               *m_boundPattern{nullptr};

    // ── Group-level properties on the same property browser ─────────────
    QtVariantProperty *m_groupVariant{nullptr};
    QMap<QtProperty *, QString> m_groupPropKeys;
    QMap<QString, QtVariantProperty *> m_groupProps;


    // ── Test image ──────────────────────────────────────────────────────
    cv::Mat m_currentImage;

    // ── Pattern thumbnail scene ────────────────────────────────────────
    QGraphicsScene      *m_thumbScene  {nullptr};
    QGraphicsPixmapItem *m_thumbPixmap {nullptr};
};

#endif // LOCALIZATION_PATTERNS_WIDGET_H
