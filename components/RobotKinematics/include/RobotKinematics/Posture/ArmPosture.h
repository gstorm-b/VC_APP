#pragma once

#include <map>
#include <optional>
#include <string>

namespace RobotKinematics {

// Generic posture/branch descriptor for serial arms. The shoulder/elbow/wrist branch
// values are conventionally -1 or +1; vendor-specific branches are kept separately.
// Posture names (lefty/righty, above/below, flip/non-flip) are NOT hard-coded here;
// they live in preset/posture metadata and are mapped by a PostureResolver (Phase 4).
struct ArmPosture {
    std::optional<int> shoulder;
    std::optional<int> elbow;
    std::optional<int> wrist;
    std::map<std::string, int> vendorSpecific;

    bool isEmpty() const
    {
        return !shoulder.has_value() && !elbow.has_value() && !wrist.has_value()
               && vendorSpecific.empty();
    }
};

} // namespace RobotKinematics
