#pragma once

#include <RobotKinematics/Collision/CollisionGeometry.h>
#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace RobotKinematics {

enum class MeshCollisionBackendKind {
    None,
    Coal,
    Fcl,
    VtkDebug
};

enum class MeshFileFormat {
    Stl
};

enum class MeshSourceUnits {
    Meters,
    Millimeters
};

enum class MeshQualityMode {
    Original,
    Simplified,
    Convex
};

std::string toString(MeshCollisionBackendKind backendKind);
Result<MeshCollisionBackendKind> meshCollisionBackendKindFromString(const std::string& value);

struct MeshCollisionQuality {
    MeshQualityMode mode = MeshQualityMode::Original;
    std::optional<std::size_t> triangleCount;
    std::optional<std::string> simplifiedFrom;
    std::optional<double> maxSimplificationError_m;
};

struct MeshCollisionGeometry {
    std::string id;
    std::string linkId;
    std::string path;
    MeshFileFormat format = MeshFileFormat::Stl;
    MeshSourceUnits sourceUnits = MeshSourceUnits::Meters;
    double scaleToMeters = 1.0;
    Pose meshToLink = Pose::identity();
    double margin_m = 0.0;
    bool enabled = true;
    MeshCollisionQuality quality;
};

struct MeshCollisionProfile {
    std::string id;
    std::string robotModel;
    std::vector<MeshCollisionBackendKind> backendPreference;
    std::vector<MeshCollisionGeometry> meshes;
    std::vector<DisabledCollisionPair> disabledPairs;
    std::vector<SourceReference> sources;
    std::map<std::string, std::string> metadata;
};

} // namespace RobotKinematics
