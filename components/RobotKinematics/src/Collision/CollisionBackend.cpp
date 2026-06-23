#include <RobotKinematics/Collision/CollisionBackend.h>
#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
#include "CoalMeshCollisionBackend.h"
#endif

namespace RobotKinematics {

namespace {
std::string requestedMeshBackendName(const MeshCollisionProfile& profile,
                                     const MeshCollisionCheckRequest& request)
{
    if (request.preferredBackend != MeshCollisionBackendKind::None) {
        return toString(request.preferredBackend);
    }
    if (!profile.backendPreference.empty()) {
        return toString(profile.backendPreference.front());
    }
    return "none";
}

MeshCollisionBackendKind selectCompiledBackend(const MeshCollisionProfile& profile,
                                               const MeshCollisionCheckRequest& request)
{
#ifndef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
    (void)profile;
#endif

    if (request.preferredBackend != MeshCollisionBackendKind::None) {
#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
        if (request.preferredBackend == MeshCollisionBackendKind::Coal) {
            return MeshCollisionBackendKind::Coal;
        }
#endif
        return MeshCollisionBackendKind::None;
    }

#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
    for (const MeshCollisionBackendKind backend : profile.backendPreference) {
        if (backend == MeshCollisionBackendKind::Coal) {
            return MeshCollisionBackendKind::Coal;
        }
    }
#endif

#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
    return MeshCollisionBackendKind::Coal;
#else
    return MeshCollisionBackendKind::None;
#endif
}
}

CollisionBackendInfo CollisionBackends::primitiveInfo()
{
    return CollisionBackendInfo{
        CollisionBackendKind::Primitive,
        MeshCollisionBackendKind::None,
        "primitive_sphere_capsule",
        true,
        false,
        true,
        false,
        "Built-in sphere/capsule self-collision backend",
    };
}

CollisionBackendInfo CollisionBackends::meshInfo()
{
#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
    return coalMeshBackendInfo();
#else
    return CollisionBackendInfo{
        CollisionBackendKind::Mesh,
        MeshCollisionBackendKind::None,
        "mesh_unavailable",
        false,
        false,
        false,
        false,
        "No mesh collision backend is compiled into this build",
    };
#endif
}

std::vector<CollisionBackendInfo> CollisionBackends::availableBackends()
{
    return {primitiveInfo(), meshInfo()};
}

CollisionCheckResult CollisionBackends::checkPrimitive(const SerialRobotConfig& config,
                                                       const CollisionProfile& profile,
                                                       const CollisionCheckRequest& request)
{
    return CollisionChecker::check(config, profile, request);
}

CollisionCheckResult CollisionBackends::checkMesh(const SerialRobotConfig& config,
                                                  const MeshCollisionProfile& profile,
                                                  const MeshCollisionCheckRequest& request)
{
#ifdef ROBOTKINEMATICS_HAVE_COAL_MESH_BACKEND
    if (selectCompiledBackend(profile, request) == MeshCollisionBackendKind::Coal) {
        return checkCoalMeshBackend(config, profile, request);
    }
#else
    (void)config;
#endif

    CollisionCheckResult result;
    result.status = KinematicsStatus::UnsupportedSolver;
    result.hasCollision = false;
    result.message =
        "requested mesh collision backend is not available in this build; requested backend preference: " +
        requestedMeshBackendName(profile, request);
    return result;
}

} // namespace RobotKinematics
