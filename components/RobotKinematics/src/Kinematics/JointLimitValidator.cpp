#include <RobotKinematics/Kinematics/JointLimitValidator.h>

namespace RobotKinematics {

namespace {
constexpr double kLimitTolerance = 1e-9;

int movableJointCount(const SerialRobotConfig& config)
{
    int count = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            ++count;
        }
    }
    return count;
}
}

JointLimitCheck JointLimitValidator::validate(const SerialRobotConfig& config, const JointVector& joints)
{
    JointLimitCheck result;

    if (joints.size() != movableJointCount(config)) {
        result.status = KinematicsStatus::JointDimensionMismatch;
        return result;
    }

    int movableIndex = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type != JointType::Revolute && joint.type != JointType::Prismatic) {
            continue;
        }

        const double value = joints[movableIndex];
        if (joint.limits.has_value()) {
            const JointLimits& limits = *joint.limits;
            if (value < limits.lower - kLimitTolerance || value > limits.upper + kLimitTolerance) {
                JointLimitViolation violation;
                violation.jointIndex = movableIndex;
                violation.jointId = joint.id;
                violation.value = value;
                violation.lower = limits.lower;
                violation.upper = limits.upper;
                result.violations.push_back(violation);
            }
        }
        ++movableIndex;
    }

    if (!result.violations.empty()) {
        result.status = KinematicsStatus::JointLimitViolation;
    }
    return result;
}

} // namespace RobotKinematics
