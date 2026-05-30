#ifndef MATCH_PATTERN_CONFIG_H
#define MATCH_PATTERN_CONFIG_H

#include "imatch_type_config.h"
#include "edge_match_config.h"
#include <memory>
#include <string>
#include <opencv2/core.hpp>

namespace mtc {

// ---------------------------------------------------------------------------
// MatchPatternConfig — per-pattern configuration carried by MatchPattern.
//
// Holds type-agnostic identity + search parameters.  Algorithm-specific
// parameters live in `typeConfig` (default: EdgeMatchConfig).
//
// Typed access helpers (edgeConfig / configAs<T>) return nullptr when the
// current type does not match, so callers can guard safely.
//
// Copy semantics: deep-copies `typeConfig` via clone() so each pattern owns
// an independent copy of its algorithm config.
// ---------------------------------------------------------------------------
struct MatchPatternConfig {
    MatchPatternConfig();
    MatchPatternConfig(const MatchPatternConfig& other);
    MatchPatternConfig& operator=(const MatchPatternConfig& other);
    MatchPatternConfig(MatchPatternConfig&&)            = default;
    MatchPatternConfig& operator=(MatchPatternConfig&&) = default;

    // ── Identity ─────────────────────────────────────────────────────────
    std::wstring m_patternName;
    int          m_patternIndex = 0;

    // ── Training image ────────────────────────────────────────────────────
    cv::Mat m_rawImage;

    // ── Type-agnostic search parameters ───────────────────────────────────
    double m_minScore       = 0.9;
    double m_angle          = 0.0;
    double m_toleranceAngle = 180.0;
    double m_maxOverlap     = 0.1;
    /// TODO: change toleranceAngle parameters
    // double m_toleranceAngleMin = -180.0;
    // double m_toleranceAngleMax = 180.0;

    // ── Pick position ─────────────────────────────────────────────────────
    cv::Point2f m_pickPosition;

    // ── Algorithm-specific config ─────────────────────────────────────────
    // Default-constructed as EdgeMatchConfig.  Replace via setMatchingType()
    // or assign a pre-built config directly.
    std::shared_ptr<IMatchTypeConfig> typeConfig;

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

} // namespace mtc
#endif // MATCH_PATTERN_CONFIG_H
