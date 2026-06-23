#pragma once

#include <RobotKinematics/Collision/CollisionProfile.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

struct CollisionProfileValidationIssue {
    KinematicsStatus status = KinematicsStatus::InvalidRobotConfig;
    std::string field;
    std::string message;
};

struct CollisionProfileValidationResult {
    std::vector<CollisionProfileValidationIssue> issues;

    bool ok() const;
    KinematicsStatus status() const;
};

class CollisionProfileValidator
{
public:
    // Same-link geometry is allowed in the profile, but the runtime checker may skip
    // same-link pairs unless a later scope explicitly enables them.
    // Adjacent contacts should be modeled via explicit disabledPairs with a reason such as
    // adjacent_joint_contact so the runtime filter is deterministic and reviewable.
    static CollisionProfileValidationResult validate(const SerialRobotConfig& config,
                                                     const CollisionProfile& profile);
};

} // namespace RobotKinematics
