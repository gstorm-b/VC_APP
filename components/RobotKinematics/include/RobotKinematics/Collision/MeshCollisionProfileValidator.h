#pragma once

#include <RobotKinematics/Collision/MeshCollisionProfile.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

struct MeshCollisionProfileValidationIssue {
    KinematicsStatus status = KinematicsStatus::InvalidRobotConfig;
    std::string field;
    std::string message;
};

struct MeshCollisionProfileValidationResult {
    std::vector<MeshCollisionProfileValidationIssue> issues;

    bool ok() const;
    KinematicsStatus status() const;
};

class MeshCollisionProfileValidator
{
public:
    static MeshCollisionProfileValidationResult validate(const SerialRobotConfig& config,
                                                         const MeshCollisionProfile& profile);
};

} // namespace RobotKinematics
