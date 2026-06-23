#pragma once

#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>

namespace RobotKinematics {

class UrdfAdapter
{
public:
    static Result<std::string> exportSerialRobot(const SerialRobotConfig& config);
    static Result<SerialRobotConfig> importSerialRobot(const std::string& urdf,
                                                       const std::string& baseLinkId,
                                                       const std::string& flangeLinkId);
};

} // namespace RobotKinematics
