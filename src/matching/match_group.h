#ifndef MATCH_GROUP_H
#define MATCH_GROUP_H

#include "match_pattern.h"
#include "manager_result.h"
#include "edge_match_config.h"

#include <memory>

namespace mtc {

class PatternGroupManager;

// ---------------------------------------------------------------------------
// MatchGroupConfig — per-group configuration.
//
// Holds group identity and the algorithm-specific config (`typeConfig`,
// default EdgeMatchConfig) that is SHARED by every pattern in the group.
// Typed access helpers (edgeConfig / configAs<T>) return nullptr when the
// current matching type does not match.
//
// Picking / collision-box geometry is per-pattern — it lives on
// MatchPatternConfig, not here (collision is an Edge-Based, per-pattern
// concern).
//
// Copy semantics: deep-copies `typeConfig` via clone() so each group owns an
// independent copy of its algorithm config.
// ---------------------------------------------------------------------------
class MatchGroupConfig {
public:
    MatchGroupConfig();
    MatchGroupConfig(const MatchGroupConfig &other);
    MatchGroupConfig &operator=(const MatchGroupConfig &other);
    MatchGroupConfig(MatchGroupConfig &&)            = default;
    MatchGroupConfig &operator=(MatchGroupConfig &&) = default;

    std::wstring m_groupName;
    int m_groupIndex{0};
    bool m_sortByAngle{false};
    double m_sortConditionAngle{0.0};

    // ── Algorithm-specific config — shared by all patterns in the group ───
    // Default-constructed as EdgeMatchConfig.  Replace via setMatchingType().
    std::shared_ptr<IMatchTypeConfig> typeConfig;

    std::vector<MatchPatternConfig> m_patterns;

    // ── Typed helpers ─────────────────────────────────────────────────────
    MatchingType matchingType() const noexcept {
        return typeConfig ? typeConfig->type() : MatchingType::EdgeBased;
    }

    // Switches to a different algorithm type; resets typeConfig to defaults.
    void setMatchingType(MatchingType t) {
        if (!typeConfig || typeConfig->type() != t)
            typeConfig = IMatchTypeConfig::createDefault(t);
    }

    // Convenience cast — returns nullptr if current type != EdgeBased.
    EdgeMatchConfig*       edgeConfig()       noexcept { return dynamic_cast<EdgeMatchConfig*>(typeConfig.get()); }
    const EdgeMatchConfig* edgeConfig() const noexcept { return dynamic_cast<const EdgeMatchConfig*>(typeConfig.get()); }

    // Generic typed cast for future algorithm types.
    template<typename T>       T* configAs()       noexcept { return dynamic_cast<T*>(typeConfig.get()); }
    template<typename T> const T* configAs() const noexcept { return dynamic_cast<const T*>(typeConfig.get()); }
};

class MatchGroup {
public:
    MatchGroup();
    MatchGroup(std::wstring name, int index);
    ~MatchGroup();

    void cloneConfigTo(MatchGroup &group);
    const MatchGroupConfig &config() const { return m_config; }
    const std::wstring &name() const { return m_config.m_groupName; }
    int number() const { return m_config.m_groupIndex; }

    std::vector<std::shared_ptr<MatchPattern>> patterns() const;
    MatchPattern* findPatternByName(const std::wstring &name) const;
    MatchPattern* findPatternByNumber(int number) const;

    bool containsPatternName(const std::wstring &name) const;
    bool containsPatternNumber(int number) const;
    int patternCount() const;
    bool isEmpty() const;

    // ── Pattern mutations ─────────────────────────────────────────────────────
    /// Create a new Pattern with the given config.
    /// Fails if name or number already exists in this group.
    ManagerResult addPattern(const MatchPatternConfig &config);

    /// Remove by name.
    ManagerResult removePattern(const std::wstring &patternName);

    /// Convenience: remove by number.
    ManagerResult removePatternByNumber(int number);

    std::shared_ptr<MatchPattern> lastPatternAccess() const;

    /// Replace the entire config of a pattern identified by its current name.
    /// Validates that the new name/number do not collide with siblings.
    ManagerResult setPatternConfig(const std::wstring &currentName,
                                   const MatchPatternConfig &newConfig);

    /// Targeted field setters — avoids shipping the whole config just to rename.
    ManagerResult renamePattern  (const std::wstring &currentName, const std::wstring &newName);
    ManagerResult renumberPattern(const std::wstring &patternName, int newNumber);
    ManagerResult setPatternImage(const std::wstring &patternName, const cv::Mat &image);

    static bool validateIndexRange(int index);
    static bool setIndexRange(int min, int max);

    static int getMaxGroupRange();
    static int getMinGroupRange();
    static int getMaxPatternRange();
    static int getMinPatternRange();

    static int m_min_index_range;
    static int m_max_index_range;
    static int m_min_pattern_range;
    static int m_max_pattern_range;

private:
    friend class PatternGroupManager;
    explicit MatchGroup(const MatchGroupConfig &config,
                          PatternGroupManager *parent);

    // Called only by PatternGroupManager after uniqueness check at manager level.
    ManagerResult setConfig(const MatchGroupConfig &cfg);
    ManagerResult setName(const std::wstring &name);
    ManagerResult setNumber(int number);

    // ── Helpers ───────────────────────────────────────────────────────────────
    ManagerResult validatePatternConfig(const MatchPatternConfig &cfg,
                                        const std::wstring &excludeName = {}) const;

private:
    PatternGroupManager *m_manager;
    MatchGroupConfig m_config;
    std::vector<std::shared_ptr<MatchPattern>> m_patterns;
    std::shared_ptr<MatchPattern> m_lastPatternAccess;
};

}

#endif // MATCH_GROUP_H
