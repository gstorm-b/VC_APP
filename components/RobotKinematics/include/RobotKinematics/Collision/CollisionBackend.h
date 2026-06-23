#pragma once

#include <RobotKinematics/Collision/CollisionChecker.h>
#include <RobotKinematics/Collision/MeshCollisionProfile.h>
#include <RobotKinematics/Core/Result.h>

#include <string>
#include <vector>

namespace RobotKinematics {

enum class CollisionBackendKind {
    Primitive,
    Mesh
};

struct CollisionBackendInfo {
    CollisionBackendKind kind = CollisionBackendKind::Primitive;
    MeshCollisionBackendKind meshBackend = MeshCollisionBackendKind::None;
    std::string backendName;
    bool available = false;
    bool supportsMesh = false;
    bool supportsDistance = false;
    bool supportsContacts = false;
    std::string detail;
};

struct MeshCollisionCheckRequest {
    JointVector joints;
    double safetyMargin_m = 0.0;
    bool returnAllPairs = true;
    MeshCollisionBackendKind preferredBackend = MeshCollisionBackendKind::None;
};

class CollisionBackends
{
public:
    static CollisionBackendInfo primitiveInfo();
    static CollisionBackendInfo meshInfo();
    static std::vector<CollisionBackendInfo> availableBackends();

    static CollisionCheckResult checkPrimitive(const SerialRobotConfig& config,
                                               const CollisionProfile& profile,
                                               const CollisionCheckRequest& request);

    static CollisionCheckResult checkMesh(const SerialRobotConfig& config,
                                          const MeshCollisionProfile& profile,
                                          const MeshCollisionCheckRequest& request);
};

} // namespace RobotKinematics
