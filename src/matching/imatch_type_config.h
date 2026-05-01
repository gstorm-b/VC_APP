#ifndef IMATCH_TYPE_CONFIG_H
#define IMATCH_TYPE_CONFIG_H

#include "matching_types.h"
#include <memory>

namespace mtc {

// ---------------------------------------------------------------------------
// IMatchTypeConfig — abstract base for per-algorithm runtime/learn parameters.
//
// Each algorithm family (EdgeBased, Correlation, …) provides exactly one
// concrete subclass.  The interface is kept Qt-free so the core matching
// library has no UI dependency.  Property-browser integration lives entirely
// in MatchConfigPropertyAdapter.
// ---------------------------------------------------------------------------
class IMatchTypeConfig {
public:
    virtual ~IMatchTypeConfig() = default;

    virtual MatchingType type() const noexcept = 0;

    // Deep-copy factory — used when MatchPatternConfig is copied by value.
    virtual std::unique_ptr<IMatchTypeConfig> clone() const = 0;

    // Factory: construct a default-initialised config for a given type.
    // Returns nullptr for types that are not yet implemented.
    static std::unique_ptr<IMatchTypeConfig> createDefault(MatchingType t);
};

} // namespace mtc
#endif // IMATCH_TYPE_CONFIG_H
