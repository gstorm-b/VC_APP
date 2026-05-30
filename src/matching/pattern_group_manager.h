#ifndef PATTERN_GROUP_MANAGER_H
#define PATTERN_GROUP_MANAGER_H

#include <QObject>
#include <QList>
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "match_group.h"
#include "manager_result.h"

namespace mtc {

class PatternGroupManager : public QObject {
    Q_OBJECT

public:
    explicit PatternGroupManager(QObject *parent = nullptr);
    ~PatternGroupManager();

    QList<std::shared_ptr<MatchGroup>> groups() const;
    std::shared_ptr<MatchGroup> findGroupByName(const QString &name) const;
    std::shared_ptr<MatchGroup> findGroupByNumber(int number) const;

    bool containsGroupName(const QString &name) const;
    bool containsGroupNumber(int number) const;
    int groupCount() const;

    /// Create a new group. Fails if name or number already exists.
    ManagerResult addGroup(const MatchGroupConfig &config);

    /// Remove the group with the given name (and all its patterns).
    ManagerResult removeGroup(const QString &groupName);

    /// Convenience: remove by number.
    ManagerResult removeGroupByNumber(int number);

    /// Replace the full config of the group identified by currentName.
    ManagerResult setGroupConfig(const QString &currentName,
                                 const MatchGroupConfig &newConfig);

    ManagerResult renameGroup  (const QString &currentName, const QString &newName);
    ManagerResult renumberGroup(const QString &groupName, int newNumber);

    ManagerResult addPattern   (const QString &groupName, const MatchPatternConfig &config);
    ManagerResult removePattern(const QString &groupName, const QString &patternName);

    ManagerResult setPatternConfig  (const QString &groupName,
                                   const QString &currentPatternName,
                                   const MatchPatternConfig &newConfig);
    ManagerResult renamePattern     (const QString &groupName,
                                const QString &currentName,
                                const QString &newName);
    ManagerResult renumberPattern   (const QString &groupName,
                                  const QString &patternName,
                                  int newNumber);
    ManagerResult setPatternImage   (const QString &groupName,
                                  const QString &patternName,
                                  const cv::Mat &image);

    // =========================================================================
    //  Bulk operations
    // =========================================================================

    /// Sort all groups in-place by number ascending and re-emit groupsReordered.
    void sortGroupsByNumber();

    /// Sort patterns inside a single group by number ascending.
    ManagerResult sortPatternsByNumber(const QString &groupName);


    // ── Serialisation helpers
    QJsonObject toJson() const;
    bool fromJson(const QJsonObject &obj);

signals:
    // ── Group-level ──────────────────────────────────────────────────────────
    void groupAdded     (std::shared_ptr<MatchGroup> group);
    void groupRemoved   (const MatchGroupConfig &removedConfig);
    void groupChanged   (std::shared_ptr<MatchGroup> group, const QString &field);
    void groupsReordered(const QList<std::shared_ptr<MatchGroup>> &newOrder);

    // ── Pattern-level (bubbled from PatternGroup) ─────────────────────────────
    void patternAdded   (MatchGroup *group, MatchPattern *pattern);
    void patternRemoved (MatchGroup *group, const MatchPatternConfig &removedConfig);
    void patternChanged (MatchGroup *group, MatchPattern *pattern, const QString &field);

private:
    ManagerResult validateGroupConfig(const MatchGroupConfig &cfg,
                                      const QString &excludeName = {}) const;

private:

    QList<std::shared_ptr<MatchGroup>> m_groups;
};

}

#endif // PATTERN_GROUP_MANAGER_H
