#pragma once

#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

struct ModelValidationIssue {
    KinematicsStatus status = KinematicsStatus::InvalidRobotConfig;
    std::string field;
    std::string message;
};

struct ModelValidationResult {
    std::vector<ModelValidationIssue> issues;

    bool ok() const;
    KinematicsStatus status() const;
};

class RobotModelValidator {
public:
    static ModelValidationResult validateSerialRobotConfig(const SerialRobotConfig& config);
};

} // namespace RobotKinematics
