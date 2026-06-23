#pragma once

#include <RobotKinematics/Collision/MeshCollisionProfile.h>
#include <RobotKinematics/Core/Result.h>

#include <string>

namespace RobotKinematics {

class MeshCollisionProfileJsonLoader
{
public:
    static Result<MeshCollisionProfile> loadFile(const std::string& path);
    static Result<MeshCollisionProfile> loadJson(const std::string& json);
};

} // namespace RobotKinematics
