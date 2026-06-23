#pragma once

#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>
#include <vector>

namespace RobotKinematics {

enum class DhJointType {
    Revolute,
    Prismatic
};

struct StandardDhParameter {
    std::string jointId;
    double a_m = 0.0;
    double alpha_rad = 0.0;
    double d_m = 0.0;
    double thetaOffset_rad = 0.0;
    DhJointType type = DhJointType::Revolute;
    JointLimits limits;
    double home = 0.0;
};

class DhAdapter
{
public:
    static Result<SerialRobotConfig> fromStandardDh(const RobotIdentity& identity,
                                                    const std::vector<StandardDhParameter>& rows);
};

} // namespace RobotKinematics
