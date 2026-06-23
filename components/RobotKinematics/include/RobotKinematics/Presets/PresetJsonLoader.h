#pragma once

#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <string>

namespace RobotKinematics {

class PresetJsonLoader
{
public:
    static Result<SerialRobotConfig> loadFile(const std::string& path);
    static Result<SerialRobotConfig> loadJson(const std::string& json);
};

} // namespace RobotKinematics
