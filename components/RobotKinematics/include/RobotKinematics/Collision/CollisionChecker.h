#pragma once

#include <RobotKinematics/Collision/CollisionProfile.h>
#include <RobotKinematics/Core/JointVector.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <cstddef>
#include <string>
#include <vector>

namespace RobotKinematics {

struct CollisionCheckRequest {
    JointVector joints;
    double safetyMargin_m = 0.0;
    bool returnAllPairs = true;
};

struct CollisionPairResult {
    std::string geometryA;
    std::string geometryB;
    std::string linkA;
    std::string linkB;
    bool colliding = false;
    double distance_m = 0.0;
    std::size_t contactCount = 0;
};

struct CollisionCheckResult {
    KinematicsStatus status = KinematicsStatus::Ok;
    bool hasCollision = false;
    std::vector<CollisionPairResult> pairs;
    std::string message;

    bool ok() const { return status == KinematicsStatus::Ok; }
};

class CollisionChecker
{
public:
    static CollisionCheckResult check(const SerialRobotConfig& config,
                                      const CollisionProfile& profile,
                                      const CollisionCheckRequest& request);
};

} // namespace RobotKinematics
