#pragma once

#include <RobotKinematics/Collision/CollisionBackend.h>

namespace RobotKinematics {

CollisionBackendInfo coalMeshBackendInfo();

CollisionCheckResult checkCoalMeshBackend(const SerialRobotConfig& config,
                                          const MeshCollisionProfile& profile,
                                          const MeshCollisionCheckRequest& request);

} // namespace RobotKinematics
