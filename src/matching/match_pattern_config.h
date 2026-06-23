#ifndef MATCH_PATTERN_CONFIG_H
#define MATCH_PATTERN_CONFIG_H

#include <string>
#include <opencv2/core.hpp>

namespace mtc {

// ---------------------------------------------------------------------------
// MatchPatternConfig — per-pattern configuration carried by MatchPattern.
//
// Holds identity, type-agnostic search parameters, and the per-pattern
// picking / collision-box geometry.  The matching-algorithm parameters
// (Canny thresholds, greediness, binarization, …) are NOT here — they live on
// the owning group's MatchGroupConfig::typeConfig and are shared by every
// pattern in the group.
//
// Note on the picking-box fields: collision detection is only produced by the
// Edge-Based algorithm, so m_pickingBox* are meaningful only when the group's
// matching type is EdgeBased; the Correlation algorithm ignores them.
//
// Copy semantics: deep-copies the training image (m_rawImage) so each pattern
// owns an independent pixel buffer.
// ---------------------------------------------------------------------------
struct MatchPatternConfig {
    MatchPatternConfig() = default;
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

    // ── Picking / collision-box geometry (Edge-Based only) ────────────────
    // Used by the collision check in ImageMatcher; ignored by Correlation.
    cv::Size2f  m_pickingBoxSize{0.0f, 0.0f};
    double      m_pickingBoxDistance{0.0};
    double      m_pickingBoxAngle{0.0};

    // ── Picking offset applied at picking time ────────────────────────────
    cv::Point3f m_pickingOffset{0.0f, 0.0f, 0.0f};
};

} // namespace mtc
#endif // MATCH_PATTERN_CONFIG_H
