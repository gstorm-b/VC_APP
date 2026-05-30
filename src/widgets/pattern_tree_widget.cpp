#include "pattern_tree_widget.h"

#include <QApplication>
#include <QTimer>
#include <algorithm>

// ============================================================================
//  Shared style constants
// ============================================================================
namespace Style {

// Dark-panel palette — adjust to match your application theme
constexpr auto BG_PRIMARY       = "#1A1B1E";
constexpr auto BG_ITEM          = "#1F2023";
constexpr auto BG_ITEM_HOVER    = "#26282C";
constexpr auto BG_ITEM_SELECTED = "#2A2D33";

constexpr auto ACCENT_BLUE      = "#3D8EFF";
constexpr auto ACCENT_GREEN     = "#2ECC71";
constexpr auto ACCENT_PURPLE    = "#9B59B6";
constexpr auto ACCENT_RED       = "#E74C3C";
constexpr auto ACCENT_RED_HOVER = "#C0392B";

constexpr auto TEXT_PRIMARY     = "#E8E9EB";
constexpr auto TEXT_SECONDARY   = "#8B8FA8";
constexpr auto TEXT_ACCENT      = "#5DADE2";

constexpr auto BORDER_SUBTLE    = "#2E3035";

// Reusable button style templates
inline QString destructiveBtn()
{
    return QStringLiteral(
               "QPushButton {"
               "  background:%1; color:#FFF;"
               "  border:none; border-radius:4px;"
               "  font-size:11px; font-weight:600;"
               "}"
               "QPushButton:hover { background:%2; }"
               "QPushButton:pressed { background:#922B21; }")
        .arg(ACCENT_RED, ACCENT_RED_HOVER);
}

inline QString primaryBtn(const char *bg, const char *bgHover)
{
    return QStringLiteral(
               "QPushButton {"
               "  background:%1; color:#FFF;"
               "  border:none; border-radius:4px;"
               "  font-size:11px; font-weight:600;"
               "}"
               "QPushButton:hover { background:%2; }"
               "QPushButton:pressed { opacity:0.8; }")
        .arg(bg, bgHover);
}

} // namespace Style


// ============================================================================
//  PatternGroupItemWidget
// ============================================================================

PatternGroupItemWidget::PatternGroupItemWidget(const MatchGroupConfig &cfg,
                                               QWidget *parent)
    : QWidget(parent), m_config(cfg)
{
    setupUi();
    refresh();
}

void PatternGroupItemWidget::setupUi()
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 5, 8, 5);
    root->setSpacing(8);

    // ── Left accent bar ───────────────────────────────────────────────────────
    auto *bar = new QFrame(this);
    bar->setFixedSize(3, 26);
    bar->setStyleSheet(
        QStringLiteral("background:%1; border-radius:2px;").arg(Style::ACCENT_BLUE));
    root->addWidget(bar, 0, Qt::AlignVCenter);

    // ── Name label ────────────────────────────────────────────────────────────
    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet(
        QStringLiteral("color:%1; font-size:13px; font-weight:700;")
            .arg(Style::TEXT_PRIMARY));
    root->addWidget(m_nameLabel);

    // ── Group number badge ────────────────────────────────────────────────────
    m_numberLabel = new QLabel(this);
    m_numberLabel->setStyleSheet(
        QStringLiteral(
            "color:%1; font-size:11px;"
            "background:%2; border-radius:3px;"
            "padding:1px 5px;")
            .arg(Style::TEXT_SECONDARY, Style::BG_PRIMARY));
    root->addWidget(m_numberLabel);

    root->addStretch();

    // ── + Pattern button ──────────────────────────────────────────────────────
    m_addPatternBtn = new QPushButton(QStringLiteral("＋ Pattern"), this);
    m_addPatternBtn->setFixedHeight(24);
    m_addPatternBtn->setCursor(Qt::PointingHandCursor);
    m_addPatternBtn->setStyleSheet(
        Style::primaryBtn(Style::ACCENT_GREEN, "#27AE60"));
    m_addPatternBtn->setContentsMargins(8, 0, 8, 0);
    root->addWidget(m_addPatternBtn);

    // ── Delete button ─────────────────────────────────────────────────────────
    m_deleteBtn = new QPushButton(QStringLiteral("✕"), this);
    m_deleteBtn->setFixedSize(24, 24);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setStyleSheet(Style::destructiveBtn());
    m_deleteBtn->setContentsMargins(8, 0, 8, 0);
    root->addWidget(m_deleteBtn);

    connect(m_deleteBtn,     &QPushButton::clicked,
            this, &PatternGroupItemWidget::deleteRequested);
    connect(m_addPatternBtn, &QPushButton::clicked,
            this, &PatternGroupItemWidget::addPatternRequested);
}

void PatternGroupItemWidget::refresh()
{
    m_nameLabel->setText(m_config.name);
    m_numberLabel->setText(QStringLiteral("# %1").arg(m_config.number));
}

void PatternGroupItemWidget::setConfig(const MatchGroupConfig &cfg)
{
    m_config = cfg;
    refresh();
}

MatchGroupConfig PatternGroupItemWidget::config() const
{
    return m_config;
}

void PatternGroupItemWidget::mousePressEvent(QMouseEvent *event)
{
    // Walk up from the widget under the cursor; if we hit a button → skip click
    QWidget *w = childAt(event->pos());
    while (w) {
        if (qobject_cast<QPushButton *>(w)) return; // absorbed by button
        w = qobject_cast<QWidget *>(w->parent());
    }
    emit clicked();
    // Do NOT call base — letting QTreeWidget handle selection via viewport
}


// ============================================================================
//  PatternItemWidget
// ============================================================================

PatternItemWidget::PatternItemWidget(const MatchPatternConfig &cfg,
                                     QWidget *parent)
    : QWidget(parent), m_config(cfg)
{
    setupUi();
    refresh();
}

void PatternItemWidget::setupUi()
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_TranslucentBackground);

    auto *root = new QHBoxLayout(this);
    // Left indent so patterns visually nest under their group
    root->setContentsMargins(32, 5, 8, 5);
    root->setSpacing(10);

    // ── Thumbnail ─────────────────────────────────────────────────────────────
    m_thumbnail = new QLabel(this);
    m_thumbnail->setFixedSize(60, 60);
    m_thumbnail->setAlignment(Qt::AlignCenter);
    m_thumbnail->setStyleSheet(
        QStringLiteral(
            "background:%1; border:1px solid %2;"
            "border-radius:5px; color:%3; font-size:9px;")
            .arg(Style::BG_PRIMARY, Style::BORDER_SUBTLE, Style::TEXT_SECONDARY));
    root->addWidget(m_thumbnail, 0, Qt::AlignVCenter);

    // ── Info column ───────────────────────────────────────────────────────────
    auto *infoBox = new QVBoxLayout();
    infoBox->setSpacing(2);
    infoBox->setContentsMargins(0, 0, 0, 0);

    m_nameLabel = new QLabel(this);
    m_nameLabel->setStyleSheet(
        QStringLiteral("color:%1; font-size:12px; font-weight:600;")
            .arg(Style::TEXT_PRIMARY));

    m_numberLabel = new QLabel(this);
    m_numberLabel->setStyleSheet(
        QStringLiteral("color:%1; font-size:10px;").arg(Style::TEXT_SECONDARY));

    m_threshLabel = new QLabel(this);
    m_threshLabel->setStyleSheet(
        QStringLiteral("color:%1; font-size:10px;").arg(Style::TEXT_ACCENT));

    infoBox->addWidget(m_nameLabel);
    infoBox->addWidget(m_numberLabel);
    infoBox->addWidget(m_threshLabel);
    infoBox->addStretch();

    root->addLayout(infoBox, 1);

    // ── Delete button ─────────────────────────────────────────────────────────
    m_deleteBtn = new QPushButton(QStringLiteral("✕"), this);
    m_deleteBtn->setFixedSize(22, 22);
    m_deleteBtn->setCursor(Qt::PointingHandCursor);
    m_deleteBtn->setStyleSheet(Style::destructiveBtn());
    m_deleteBtn->setContentsMargins(8, 0, 8, 0);

    root->addWidget(m_deleteBtn, 0, Qt::AlignVCenter);

    connect(m_deleteBtn, &QPushButton::clicked,
            this, &PatternItemWidget::deleteRequested);
}

void PatternItemWidget::refresh()
{
    m_nameLabel->setText(m_config.name);
    m_numberLabel->setText(QStringLiteral("Pattern  #%1").arg(m_config.number));
    m_threshLabel->setText(
        QStringLiteral("Thresh  %1").arg(m_config.threshScore, 0, 'f', 3));

    if (!m_config.thumbnail.isNull()) {
        m_thumbnail->setPixmap(
            m_config.thumbnail.scaled(
                m_thumbnail->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
    } else {
        m_thumbnail->setText(QStringLiteral("No\nImage"));
    }
}

void PatternItemWidget::setConfig(const MatchPatternConfig &cfg)
{
    m_config = cfg;
    refresh();
}

MatchPatternConfig PatternItemWidget::config() const
{
    return m_config;
}

void PatternItemWidget::mousePressEvent(QMouseEvent *event)
{
    QWidget *w = childAt(event->pos());
    while (w) {
        if (qobject_cast<QPushButton *>(w)) return;
        w = qobject_cast<QWidget *>(w->parent());
    }
    emit clicked();
}


// ============================================================================
//  FooterItemWidget
// ============================================================================

FooterItemWidget::FooterItemWidget(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

void FooterItemWidget::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(8, 6, 8, 6);
    root->setSpacing(8);

    m_addGroupBtn = new QPushButton(QStringLiteral("＋  Add Group"), this);
    m_addGroupBtn->setCursor(Qt::PointingHandCursor);
    m_addGroupBtn->setFixedHeight(28);
    m_addGroupBtn->setStyleSheet(
        Style::primaryBtn(Style::ACCENT_BLUE, "#2D7DD2"));

    m_autoSortBtn = new QPushButton(QStringLiteral("⇅  Auto Sort"), this);
    m_autoSortBtn->setCursor(Qt::PointingHandCursor);
    m_autoSortBtn->setFixedHeight(28);
    m_autoSortBtn->setStyleSheet(
        Style::primaryBtn(Style::ACCENT_PURPLE, "#7D3C98"));

    root->addWidget(m_addGroupBtn);
    root->addWidget(m_autoSortBtn);
    root->addStretch();

    connect(m_addGroupBtn, &QPushButton::clicked,
            this, &FooterItemWidget::addGroupRequested);
    connect(m_autoSortBtn, &QPushButton::clicked,
            this, &FooterItemWidget::autoSortRequested);
}


// ============================================================================
//  PatternTreeWidget
// ============================================================================

PatternTreeWidget::PatternTreeWidget(QWidget *parent)
    : QTreeWidget(parent)
{
    setupStyle();
    appendFooterItem();
}

// ── Public API ───────────────────────────────────────────────────────────────

void PatternTreeWidget::setGroups(const QList<MatchGroupConfig> &groups)
{
    m_groups = groups;
    rebuildTree();
    expandAll();
}

QList<MatchGroupConfig> PatternTreeWidget::groups() const
{
    return m_groups;
}

void PatternTreeWidget::addGroup(const MatchGroupConfig &group)
{
    m_groups.append(group);
    const int gi = m_groups.size() - 1;

    // Remove footer, insert new group before it, re-add footer
    if (m_footerItem) {
        takeTopLevelItem(indexOfTopLevelItem(m_footerItem));
        delete m_footerItem;
        m_footerItem = nullptr;
    }
    appendGroupItem(m_groups[gi], gi);
    appendFooterItem();

    emit groupsChanged(m_groups);
}

void PatternTreeWidget::removeGroup(int groupIndex)
{
    groupIndex = mapGroupNumber(groupIndex);

    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    m_groups.removeAt(groupIndex);
    // Defer rebuild — the call may come from a signal emitted by a child widget
    // that is still on the call stack; deleting it now would be unsafe.
    QTimer::singleShot(0, this, [this]() {
        rebuildTree();
        emit groupsChanged(m_groups);
    });
}

void PatternTreeWidget::updateGroup(int groupIndex, const MatchGroupConfig &group)
{
    groupIndex = mapGroupNumber(groupIndex);

    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    m_groups[groupIndex] = group;

    auto *treeItem = topLevelItem(groupIndex);
    if (!treeItem) return;

    if (auto *w = qobject_cast<PatternGroupItemWidget *>(itemWidget(treeItem, 0)))
        w->setConfig(group);

    // emit groupChanged(groupIndex, group);
    emit groupChanged(group.number, group);
    emit groupsChanged(m_groups);
}

void PatternTreeWidget::addPattern(int groupIndex, const MatchPatternConfig &pattern)
{
    groupIndex = mapGroupNumber(groupIndex);

    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    m_groups[groupIndex].patterns.append(pattern);
    const int pi = m_groups[groupIndex].patterns.size() - 1;

    auto *groupTreeItem = topLevelItem(groupIndex);
    if (!groupTreeItem) return;

    appendPatternItem(groupTreeItem, pattern, groupIndex, pi);
    groupTreeItem->setExpanded(true);

    // emit groupChanged(groupIndex, m_groups[groupIndex]);
    emit groupChanged(m_groups[groupIndex].number, m_groups[groupIndex]);
    emit groupsChanged(m_groups);
}

void PatternTreeWidget::removePattern(int groupIndex, int patternIndex)
{
    groupIndex = mapGroupNumber(groupIndex);

    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    auto &patterns = m_groups[groupIndex].patterns;
    if (patternIndex < 0 || patternIndex >= patterns.size()) return;
    patterns.removeAt(patternIndex);

    QTimer::singleShot(0, this, [this, groupIndex]() {
        rebuildTree();
        // emit groupChanged(groupIndex, m_groups[groupIndex]);
        emit groupChanged(m_groups[groupIndex].number, m_groups[groupIndex]);
        emit groupsChanged(m_groups);
    });
}

void PatternTreeWidget::updatePattern(int groupIndex, int patternIndex,
                                      const MatchPatternConfig &pattern)
{
    groupIndex = mapGroupNumber(groupIndex);

    if (groupIndex < 0 || groupIndex >= m_groups.size()) return;
    auto &patterns = m_groups[groupIndex].patterns;
    if (patternIndex < 0 || patternIndex >= patterns.size()) return;
    patterns[patternIndex] = pattern;

    auto *groupTreeItem = topLevelItem(groupIndex);
    if (!groupTreeItem) return;
    auto *patternTreeItem = groupTreeItem->child(patternIndex);
    if (!patternTreeItem) return;

    if (auto *w = qobject_cast<PatternItemWidget *>(itemWidget(patternTreeItem, 0)))
        w->setConfig(pattern);

    // emit patternChanged(groupIndex, patternIndex, pattern);
    emit patternChanged(m_groups[groupIndex].number, pattern.number, pattern);
    emit groupsChanged(m_groups);
}

void PatternTreeWidget::autoSort()
{
    std::sort(m_groups.begin(), m_groups.end(),
              [](const MatchGroupConfig &a, const MatchGroupConfig &b) {
                  return a.number < b.number;
              });
    rebuildTree();
    emit groupsChanged(m_groups);
}

// ── Private helpers ──────────────────────────────────────────────────────────

void PatternTreeWidget::setupStyle()
{
    setHeaderHidden(true);
    setRootIsDecorated(true);   // shows ▶/▼ for groups that have patterns
    setIndentation(0);          // visual indent is done via widget margins
    setAnimated(true);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    setStyleSheet(QStringLiteral(
                      "QTreeWidget {"
                      "  background:%1; border:none; outline:none;"
                      "}"
                      "QTreeWidget::item {"
                      "  padding:0; margin:0; border:none;"
                      "  background:transparent;"
                      "}"
                      "QTreeWidget::item:selected {"
                      "  background:%2;"
                      "}"
                      "QTreeWidget::item:hover:!selected {"
                      "  background:%3;"
                      "}"
                      /* Expand/collapse branch indicator */
                      "QTreeWidget::branch {"
                      "  background:%1;"
                      "}"
                      "QTreeWidget::branch:has-children:!has-siblings:closed,"
                      "QTreeWidget::branch:closed:has-children:has-siblings {"
                      "  image: url(none);" // rely on Qt default arrows — remove to use custom
                      "}"
                      /* Separator between groups */
                      "QTreeWidget::item[type='group'] {"
                      "  border-bottom:1px solid %4;"
                      "}"
                      )
                      .arg(Style::BG_PRIMARY,
                           Style::BG_ITEM_SELECTED,
                           Style::BG_ITEM_HOVER,
                           Style::BORDER_SUBTLE));
}

void PatternTreeWidget::rebuildTree()
{
    const QSet<int> expanded = expandedGroupIndices();

    // Detach footer before clearing so we can delete it cleanly
    if (m_footerItem) {
        takeTopLevelItem(indexOfTopLevelItem(m_footerItem));
        delete m_footerItem;
        m_footerItem = nullptr;
    }
    clear();

    for (int gi = 0; gi < m_groups.size(); ++gi)
        appendGroupItem(m_groups[gi], gi);

    appendFooterItem();
    restoreExpanded(expanded);
}

void PatternTreeWidget::appendGroupItem(const MatchGroupConfig &group, int groupIndex)
{
    auto *treeItem = new QTreeWidgetItem();
    treeItem->setData(0, Qt::UserRole,     QStringLiteral("group"));
    treeItem->setData(0, Qt::UserRole + 1, groupIndex);
    treeItem->setSizeHint(0, QSize(0, 38));

    // Insert before footer (footer is always last)
    const int pos = m_footerItem
                        ? indexOfTopLevelItem(m_footerItem)
                        : topLevelItemCount();
    insertTopLevelItem(pos, treeItem);

    auto *w = new PatternGroupItemWidget(group, this);
    setItemWidget(treeItem, 0, w);

    // ── Group widget → tree signals ──────────────────────────────────────────
    connect(w, &PatternGroupItemWidget::clicked, this,
            [this, group, w]() {
                emit groupClicked(group.number, w->config());
            });

    connect(w, &PatternGroupItemWidget::addPatternRequested, this,
            [this, group]() {
                emit addPatternRequested(group.number);
            });

    connect(w, &PatternGroupItemWidget::deleteRequested, this,
            [this, group]() {
                // Emit intent first (so host can veto or react before removal)
                emit deleteGroupRequested(group.number);
                // // Then apply: deferred to avoid destroying widget mid-signal
                // QTimer::singleShot(0, this, [this, groupIndex]() {
                //     if (groupIndex >= 0 && groupIndex < m_groups.size()) {
                //         m_groups.removeAt(groupIndex);
                //         rebuildTree();
                //         emit groupsChanged(m_groups);
                //     }
                // });
            });

    // ── Patterns ─────────────────────────────────────────────────────────────
    for (int pi = 0; pi < group.patterns.size(); ++pi)
        appendPatternItem(treeItem, group.patterns[pi], groupIndex, pi);
}

void PatternTreeWidget::appendPatternItem(QTreeWidgetItem *parentItem,
                                          const MatchPatternConfig &pattern,
                                          int groupIndex, int patternIndex)
{
    auto *treeItem = new QTreeWidgetItem(parentItem);
    treeItem->setData(0, Qt::UserRole,     QStringLiteral("pattern"));
    treeItem->setData(0, Qt::UserRole + 1, groupIndex);
    treeItem->setData(0, Qt::UserRole + 2, patternIndex);
    treeItem->setSizeHint(0, QSize(0, 74));

    auto *w = new PatternItemWidget(pattern, this);
    setItemWidget(treeItem, 0, w);

    int patternIdx = w->config().number;

    connect(w, &PatternItemWidget::clicked, this,
            [this, groupIndex, patternIdx, w]() {
                emit patternClicked(m_groups[groupIndex].number, patternIdx, w->config());
            });

    connect(w, &PatternItemWidget::deleteRequested, this,
            [this, groupIndex, patternIndex, patternIdx]() {
                emit deletePatternRequested(m_groups[groupIndex].number, patternIdx);
                // QTimer::singleShot(0, this, [this, groupIndex, patternIndex]() {
                //     if (groupIndex < m_groups.size()) {
                //         auto &patterns = m_groups[groupIndex].patterns;
                //         if (patternIndex < patterns.size()) {
                //             patterns.removeAt(patternIndex);
                //             rebuildTree();
                //             // emit groupChanged(groupIndex, m_groups[groupIndex]);
                //             emit groupChanged(m_groups[groupIndex].number, m_groups[groupIndex]);
                //             emit groupsChanged(m_groups);
                //         }
                //     }
                // });
            });
}

void PatternTreeWidget::appendFooterItem()
{
    m_footerItem = new QTreeWidgetItem();
    m_footerItem->setData(0, Qt::UserRole, QStringLiteral("footer"));
    // Footer must not be selectable
    m_footerItem->setFlags(m_footerItem->flags()
                           & ~Qt::ItemIsSelectable
                           & ~Qt::ItemIsEnabled);
    m_footerItem->setFlags(m_footerItem->flags() | Qt::ItemIsEnabled); // keep enabled for widget events
    m_footerItem->setSizeHint(0, QSize(0, 44));

    addTopLevelItem(m_footerItem);

    auto *w = new FooterItemWidget(this);
    setItemWidget(m_footerItem, 0, w);

    connect(w, &FooterItemWidget::addGroupRequested,
            this, &PatternTreeWidget::addGroupRequested);
    connect(w, &FooterItemWidget::autoSortRequested,
            this, &PatternTreeWidget::autoSort);
}

int PatternTreeWidget::mapGroupNumber(int groupIndex) {
    int map_number = -1;
    for (int idx=0;idx<m_groups.size();idx++) {
        if (m_groups[idx].number == groupIndex) {
            map_number = idx;
            break;
        }
    }
    return map_number;
}

int PatternTreeWidget::mapPatternNumber(int patternIndex, QList<MatchPatternConfig> &cfgs) {
    int map_number = -1;
    for (int idx=0;idx<cfgs.size();idx++) {
        if (cfgs[idx].number == patternIndex) {
            map_number = idx;
            break;
        }
    }
    return map_number;
}

QSet<int> PatternTreeWidget::expandedGroupIndices() const
{
    QSet<int> result;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        auto *item = topLevelItem(i);
        if (item && item != m_footerItem && item->isExpanded())
            result.insert(i);
    }
    return result;
}

void PatternTreeWidget::restoreExpanded(const QSet<int> &indices)
{
    for (int idx : indices) {
        auto *item = topLevelItem(idx);
        if (item && item != m_footerItem)
            item->setExpanded(true);
    }
}
