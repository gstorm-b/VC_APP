#pragma once

#include <RobotKinematics/Collision/CollisionProfile.h>
#include <RobotKinematics/Core/Result.h>

#include <string>

namespace RobotKinematics {

class CollisionProfileJsonLoader
{
public:
    static Result<CollisionProfile> loadFile(const std::string& path);
    static Result<CollisionProfile> loadJson(const std::string& json);
};

} // namespace RobotKinematics
