#pragma once

#include <RobotKinematics/Core/JointVector.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

struct JointLimitViolation {
    int jointIndex = -1;
    std::string jointId;
    double value = 0.0; // SI (rad for revolute, m for prismatic)
    double lower = 0.0;
    double upper = 0.0;
};

struct JointLimitCheck {
    KinematicsStatus status = KinematicsStatus::Ok;
    std::vector<JointLimitViolation> violations;

    bool ok() const { return status == KinematicsStatus::Ok; }
};

// Validates a joint vector's dimension and per-joint position limits against a config.
class JointLimitValidator {
public:
    static JointLimitCheck validate(const SerialRobotConfig& config, const JointVector& joints);
};

} // namespace RobotKinematics
