#ifndef MATCHING_TYPES_H
#define MATCHING_TYPES_H

#include <cstdint>

namespace mtc {

// ---------------------------------------------------------------------------
// MatchingType — discriminates which algorithm family is in use.
//
// Adding a new algorithm type:
//   1. Add a value here.
//   2. Create a concrete IMatchTypeConfig subclass.
//   3. Add a branch in IMatchTypeConfig::createDefault().
//   4. Add a branch in MatchConfigPropertyAdapter::buildTypeGroup().
// ---------------------------------------------------------------------------
enum class MatchingType : uint8_t {
    EdgeBased   = 0,  ///< Gradient-direction edge template matching (SIMD-accelerated)
    Correlation = 1,  ///< Normalised cross-correlation (reserved — not yet implemented)
};

inline const char* matchingTypeName(MatchingType t) noexcept {
    switch (t) {
    case MatchingType::EdgeBased:   return "Edge-Based";
    case MatchingType::Correlation: return "Correlation";
    }
    return "Unknown";
}

} // namespace mtc
#endif // MATCHING_TYPES_H
