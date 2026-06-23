#include "match_pattern_config.h"

namespace mtc {

MatchPatternConfig::MatchPatternConfig(const MatchPatternConfig& other) {
    m_patternName        = other.m_patternName;
    m_patternIndex       = other.m_patternIndex;
    m_rawImage           = other.m_rawImage.clone();
    m_minScore           = other.m_minScore;
    m_angle              = other.m_angle;
    m_toleranceAngle     = other.m_toleranceAngle;
    m_maxOverlap         = other.m_maxOverlap;
    m_pickPosition       = other.m_pickPosition;
    m_pickingBoxSize     = other.m_pickingBoxSize;
    m_pickingBoxDistance = other.m_pickingBoxDistance;
    m_pickingBoxAngle    = other.m_pickingBoxAngle;
    m_pickingOffset      = other.m_pickingOffset;
}

MatchPatternConfig& MatchPatternConfig::operator=(const MatchPatternConfig& other) {
    if (this != &other) {
        m_patternName        = other.m_patternName;
        m_patternIndex       = other.m_patternIndex;
        m_rawImage           = other.m_rawImage.clone();
        m_minScore           = other.m_minScore;
        m_angle              = other.m_angle;
        m_toleranceAngle     = other.m_toleranceAngle;
        m_maxOverlap         = other.m_maxOverlap;
        m_pickPosition       = other.m_pickPosition;
        m_pickingBoxSize     = other.m_pickingBoxSize;
        m_pickingBoxDistance = other.m_pickingBoxDistance;
        m_pickingBoxAngle    = other.m_pickingBoxAngle;
        m_pickingOffset      = other.m_pickingOffset;
    }
    return *this;
}

} // namespace mtc
