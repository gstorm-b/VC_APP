#pragma once

#include <RobotKinematics/Collision/CollisionGeometry.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <map>
#include <string>
#include <vector>

namespace RobotKinematics {

struct CollisionProfile {
    std::string id;
    std::string robotModel;
    std::vector<CollisionGeometry> geometries;
    std::vector<DisabledCollisionPair> disabledPairs;
    std::vector<SourceReference> sources;
    std::map<std::string, std::string> metadata;
};

} // namespace RobotKinematics
