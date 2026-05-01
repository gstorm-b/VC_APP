#include "pattern_group_manager.h"
#include "setting_keys.h"
#include "vision_utils.h"

namespace mtc {

PatternGroupManager::PatternGroupManager(QObject *parent)
    : QObject{parent} {

}

PatternGroupManager::~PatternGroupManager() {

}

// ── Group queries ─────────────────────────────────────────────────────────────
QList<std::shared_ptr<MatchGroup>> PatternGroupManager::groups() const {
    return m_groups;
}

std::shared_ptr<MatchGroup> PatternGroupManager::findGroupByName(const QString &name) const {
    for (std::shared_ptr<MatchGroup> g : m_groups)
        if (QString::fromStdWString(g->name()) == name) return g;
    return nullptr;
}

std::shared_ptr<MatchGroup> PatternGroupManager::findGroupByNumber(int number) const {
    for (std::shared_ptr<MatchGroup> g : m_groups)
        if (g->number() == number) return g;
    return nullptr;
}

bool PatternGroupManager::containsGroupName(const QString &name) const {
    return findGroupByName(name) != nullptr;
}

bool PatternGroupManager::containsGroupNumber(int number) const {
    return findGroupByNumber(number) != nullptr;
}

int PatternGroupManager::groupCount() const {
    return m_groups.size();
}

// ── Group mutations ───────────────────────────────────────────────────────────

ManagerResult PatternGroupManager::addGroup(const MatchGroupConfig &config) {
    if (auto r = validateGroupConfig(config); !r)
        return r;

    auto *group = new MatchGroup(config, this);
    // std::shared_ptr<MatchGroup> group = std::make_shared<MatchGroup>(config, this);
    std::shared_ptr<MatchGroup> group_ptr(group);
    m_groups.append(group_ptr);
    emit groupAdded(group_ptr);
    return ManagerResult::success();
}

ManagerResult PatternGroupManager::removeGroup(const QString &groupName) {
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group) {
        return ManagerResult::fail(
            tr("Group \"%1\" not found.").arg(groupName).toStdWString());
    }

    const MatchGroupConfig removedCfg = group->config();
    m_groups.removeOne(group);

    emit groupRemoved(removedCfg);
    return ManagerResult::success();
}

ManagerResult PatternGroupManager::removeGroupByNumber(int number) {
    std::shared_ptr<MatchGroup> group = findGroupByNumber(number);
    if (!group) {
        return ManagerResult::fail(
            tr("No group with number %1.").arg(number).toStdWString());
    }
    return removeGroup(QString::fromStdWString(group->name()));
}

ManagerResult PatternGroupManager::setGroupConfig(const QString &currentName,
                                                  const MatchGroupConfig &newConfig) {
    std::shared_ptr<MatchGroup> group = findGroupByName(currentName);
    if (!group) {
        return ManagerResult::fail(
            tr("Group \"%1\" not found.").arg(currentName).toStdWString());
    }

    if (auto r = validateGroupConfig(newConfig, currentName); !r)
        return r;

    return group->setConfig(newConfig);
}

ManagerResult PatternGroupManager::renameGroup(const QString &currentName,
                                               const QString &newName) {
    if (currentName == newName) {
        return ManagerResult::success();
    }

    if (newName.trimmed().isEmpty()) {
        return ManagerResult::fail(QStringLiteral("Group name must not be empty.").toStdWString());
    }

    if (containsGroupName(newName)) {
        return ManagerResult::fail(
            QStringLiteral("A group named \"%1\" already exists.").arg(newName).toStdWString());
    }

    std::shared_ptr<MatchGroup> group = findGroupByName(currentName);
    if (!group) {
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(currentName).toStdWString());
    }

    return group->setName(newName.toStdWString());
}

ManagerResult PatternGroupManager::renumberGroup(const QString &groupName,
                                                 int newNumber) {
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group) {
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());
    }

    if (group->number() == newNumber) {
        return ManagerResult::success();
    }

    if (containsGroupNumber(newNumber)) {
        return ManagerResult::fail(
            QStringLiteral("Group number %1 already exists.").arg(newNumber).toStdWString());
    }

    return group->setNumber(newNumber);
}

// ── Pattern mutations (pass-throughs) ─────────────────────────────────────────

ManagerResult PatternGroupManager::addPattern(const QString &groupName,
                                              const MatchPatternConfig &config) {
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    ManagerResult result = group->addPattern(config);
    emit patternAdded(group.get(), group->lastPatternAccess().get());
    // emit patternAdded();
    return result;
}

ManagerResult PatternGroupManager::removePattern(const QString &groupName,
                                                 const QString &patternName) {
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    return group->removePattern(patternName.toStdWString());
}

ManagerResult PatternGroupManager::setPatternConfig(const QString &groupName,
                                                    const QString &currentPatternName,
                                                    const MatchPatternConfig &newConfig) {
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    return group->setPatternConfig(currentPatternName.toStdWString(), newConfig);
}

ManagerResult PatternGroupManager::renamePattern(const QString &groupName,
                                                 const QString &currentName,
                                                 const QString &newName)
{
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    return group->renamePattern(currentName.toStdWString(), newName.toStdWString());
}

ManagerResult PatternGroupManager::renumberPattern(const QString &groupName,
                                                   const QString &patternName,
                                                   int newNumber)
{
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    return group->renumberPattern(patternName.toStdWString(), newNumber);
}

ManagerResult PatternGroupManager::setPatternImage(const QString &groupName,
                                                   const QString &patternName,
                                                   const cv::Mat &image)
{
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    return group->setPatternImage(patternName.toStdWString(), image);
}

// ── Bulk operations ───────────────────────────────────────────────────────────

void PatternGroupManager::sortGroupsByNumber()
{
    std::sort(m_groups.begin(), m_groups.end(),
              [](std::shared_ptr<MatchGroup> a, std::shared_ptr<MatchGroup> b) {
                  return a->number() < b->number();
              });
    emit groupsReordered(m_groups);
}

ManagerResult PatternGroupManager::sortPatternsByNumber(const QString &groupName)
{
    std::shared_ptr<MatchGroup> group = findGroupByName(groupName);
    if (!group)
        return ManagerResult::fail(
            QStringLiteral("Group \"%1\" not found.").arg(groupName).toStdWString());

    auto &patterns = group->m_patterns;
    std::sort(patterns.begin(), patterns.end(),
              [](std::shared_ptr<MatchPattern> a, std::shared_ptr<MatchPattern> b) {
                  return a->number() < b->number();
              });

    // Notify listeners that the group's content changed (order only).
    emit groupChanged(group, QStringLiteral("patternsReordered"));
    return ManagerResult::success();
}

// ── Private ───────────────────────────────────────────────────────────────────

ManagerResult PatternGroupManager::validateGroupConfig(
    const MatchGroupConfig &cfg, const QString &excludeName) const
{
    if (cfg.m_groupName.empty())
        return ManagerResult::fail(QStringLiteral("Group name must not be empty.").toStdWString());

    for (std::shared_ptr<MatchGroup> g : m_groups) {
        if (QString::fromStdWString(g->name()) == excludeName) continue;   // skip self
        if (g->name() == cfg.m_groupName)
            return ManagerResult::fail(
                QStringLiteral("A group named \"%1\" already exists.").arg(cfg.m_groupName).toStdWString());
        if (g->number() == cfg.m_groupIndex)
            return ManagerResult::fail(
                QStringLiteral("Group number %1 already exists.").arg(cfg.m_groupIndex).toStdWString());
    }
    return ManagerResult::success();
}

// void PatternGroupManager::savePatternGroupImage(mtc::MatchGroup &group, QString &absolute_dir) {
//     const std::vector<mtc::MatchPattern>& src = group.getPatternSource();
//     for (int ptIndex=0;ptIndex<src.size();ptIndex++) {
//         //　using wide string, to save path with kanji char
//         // save file format: absolute path/GroupName_PatternName.bmp
//         std::wstring file_name = absolute_dir.toStdWString() + L"/" +
//                                  group.getName() + L"_" +
//                                  src[ptIndex].patternConfigPtr()->m_patternName + L".bmp";
//         std::wcout << file_name;
//         vsu::imwrite_wstring(file_name, src[ptIndex].getRawImage());
//     }
// }

}
