#include "CoalMeshCollisionBackend.h"

#include <RobotKinematics/Collision/MeshCollisionProfileValidator.h>
#include <RobotKinematics/Collision/StlMeshLoader.h>
#include <RobotKinematics/Kinematics/ForwardKinematics.h>
#include <RobotKinematics/Model/RobotModelValidator.h>

#include <QDir>
#include <QFileInfo>

#include <coal/BV/OBBRSS.h>
#include <coal/BVH/BVH_model.h>
#include <coal/collision.h>
#include <coal/collision_data.h>
#include <coal/collision_object.h>
#include <coal/distance.h>

#include <cmath>
#include <iomanip>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <utility>

namespace RobotKinematics {

namespace {
using CoalBvh = coal::BVHModel<coal::OBBRSS>;

struct CachedCoalGeometry {
    std::string resolvedPath;
    TriangleMeshStatistics statistics;
    std::shared_ptr<CoalBvh> model;
};

struct CoalMeshRuntimeEntry {
    MeshCollisionGeometry mesh;
    std::shared_ptr<CachedCoalGeometry> geometry;
    std::unique_ptr<coal::CollisionObject> object;
};

struct CoalProfileRuntime {
    std::string cacheKey;
    std::vector<CoalMeshRuntimeEntry> meshes;
};

std::map<std::string, std::shared_ptr<CachedCoalGeometry>>& geometryCache()
{
    static std::map<std::string, std::shared_ptr<CachedCoalGeometry>> cache;
    return cache;
}

std::map<std::string, std::shared_ptr<CoalProfileRuntime>>& profileCache()
{
    static std::map<std::string, std::shared_ptr<CoalProfileRuntime>> cache;
    return cache;
}

std::string normalizePairKey(const std::string& a, const std::string& b)
{
    return a < b ? a + "\n" + b : b + "\n" + a;
}

void appendDouble(std::ostringstream& stream, double value)
{
    stream << std::setprecision(17) << value;
}

std::string poseKey(const Pose& pose)
{
    const Eigen::Isometry3d& iso = pose.isometry();
    std::ostringstream key;
    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            appendDouble(key, iso.linear()(row, col));
            key << ",";
        }
    }
    for (int axis = 0; axis < 3; ++axis) {
        appendDouble(key, iso.translation()(axis));
        key << ",";
    }
    return key.str();
}

std::string meshRuntimeKey(const MeshCollisionGeometry& mesh)
{
    std::ostringstream key;
    key << mesh.id << "|"
        << mesh.linkId << "|"
        << mesh.path << "|"
        << static_cast<int>(mesh.format) << "|"
        << static_cast<int>(mesh.sourceUnits) << "|";
    appendDouble(key, mesh.scaleToMeters);
    key << "|";
    appendDouble(key, mesh.margin_m);
    key << "|" << (mesh.enabled ? "1" : "0") << "|" << poseKey(mesh.meshToLink);
    return key.str();
}

std::string profileRuntimeKey(const MeshCollisionProfile& profile)
{
    std::ostringstream key;
    key << profile.id << "|" << profile.robotModel << "|";
    for (const MeshCollisionGeometry& mesh : profile.meshes) {
        key << meshRuntimeKey(mesh) << ";";
    }
    return key.str();
}

std::string resolveMeshPath(const std::string& path)
{
    const QFileInfo fileInfo(QString::fromStdString(path));
    if (fileInfo.isAbsolute()) {
        return fileInfo.absoluteFilePath().toStdString();
    }
    return QDir::current().absoluteFilePath(fileInfo.filePath()).toStdString();
}

std::string geometryCacheKey(const MeshCollisionGeometry& mesh, const std::string& resolvedPath)
{
    std::ostringstream key;
    key << resolvedPath << "|"
        << static_cast<int>(mesh.format) << "|"
        << static_cast<int>(mesh.sourceUnits) << "|";
    appendDouble(key, mesh.scaleToMeters);
    return key.str();
}

coal::Vec3s toCoalVector(const Eigen::Vector3d& value)
{
    return coal::Vec3s(value.x(), value.y(), value.z());
}

coal::Transform3s toCoalTransform(const Pose& pose)
{
    const Eigen::Isometry3d& iso = pose.isometry();
    return coal::Transform3s(iso.linear(), iso.translation());
}

CollisionCheckResult failure(KinematicsStatus status, const std::string& message)
{
    CollisionCheckResult result;
    result.status = status;
    result.hasCollision = false;
    result.message = message;
    return result;
}

Result<std::shared_ptr<CachedCoalGeometry>> loadCoalGeometry(const MeshCollisionGeometry& mesh)
{
    if (mesh.format != MeshFileFormat::Stl) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::failure(
            KinematicsStatus::InvalidRequest,
            "coal mesh backend currently supports STL meshes only");
    }

    const std::string resolvedPath = resolveMeshPath(mesh.path);
    const std::string cacheKey = geometryCacheKey(mesh, resolvedPath);

    auto& cache = geometryCache();
    const auto cached = cache.find(cacheKey);
    if (cached != cache.end()) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::success(cached->second);
    }

    // Real-world CAD STL exports often contain a handful of degenerate triangles.
    // Skip them silently so production loads do not fail on otherwise-valid assets;
    // Coal's BVH builder also expects non-degenerate faces.
    const Result<TriangleMesh> loaded =
        StlMeshLoader::loadFile(resolvedPath, StlMeshLoadOptions{mesh.scaleToMeters, false});
    if (!loaded.ok()) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::failure(
            loaded.status,
            "failed to load mesh '" + mesh.id + "' from '" + resolvedPath + "': " + loaded.message);
    }

    std::vector<coal::Vec3s> vertices;
    vertices.reserve(loaded.value.vertices_m.size());
    for (const Eigen::Vector3d& vertex : loaded.value.vertices_m) {
        vertices.push_back(toCoalVector(vertex));
    }

    std::vector<coal::Triangle32> triangles;
    triangles.reserve(loaded.value.faces.size());
    for (const TriangleFace& face : loaded.value.faces) {
        triangles.emplace_back(static_cast<coal::Triangle32::IndexType>(face.a),
                               static_cast<coal::Triangle32::IndexType>(face.b),
                               static_cast<coal::Triangle32::IndexType>(face.c));
    }

    auto model = std::make_shared<CoalBvh>();
    if (model->beginModel(static_cast<unsigned int>(triangles.size()),
                          static_cast<unsigned int>(vertices.size())) != 0) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::failure(
            KinematicsStatus::NumericalError,
            "coal beginModel failed for mesh '" + mesh.id + "'");
    }
    if (model->addSubModel(vertices, triangles) != 0) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::failure(
            KinematicsStatus::NumericalError,
            "coal addSubModel failed for mesh '" + mesh.id + "'");
    }
    if (model->endModel() != 0) {
        return Result<std::shared_ptr<CachedCoalGeometry>>::failure(
            KinematicsStatus::NumericalError,
            "coal endModel failed for mesh '" + mesh.id + "'");
    }

    auto cachedGeometry = std::make_shared<CachedCoalGeometry>();
    cachedGeometry->resolvedPath = resolvedPath;
    cachedGeometry->statistics = loaded.value.statistics;
    cachedGeometry->model = model;
    cache.insert({cacheKey, cachedGeometry});
    return Result<std::shared_ptr<CachedCoalGeometry>>::success(cachedGeometry);
}

Result<std::shared_ptr<CoalProfileRuntime>> loadProfileRuntime(const MeshCollisionProfile& profile)
{
    const std::string cacheKey = profileRuntimeKey(profile);

    auto& cache = profileCache();
    const auto cached = cache.find(cacheKey);
    if (cached != cache.end()) {
        return Result<std::shared_ptr<CoalProfileRuntime>>::success(cached->second);
    }

    auto runtime = std::make_shared<CoalProfileRuntime>();
    runtime->cacheKey = cacheKey;
    runtime->meshes.reserve(profile.meshes.size());

    for (const MeshCollisionGeometry& mesh : profile.meshes) {
        if (!mesh.enabled) {
            continue;
        }

        const Result<std::shared_ptr<CachedCoalGeometry>> geometry = loadCoalGeometry(mesh);
        if (!geometry.ok()) {
            return Result<std::shared_ptr<CoalProfileRuntime>>::failure(geometry.status, geometry.message);
        }

        CoalMeshRuntimeEntry entry;
        entry.mesh = mesh;
        entry.geometry = geometry.value;
        entry.object = std::make_unique<coal::CollisionObject>(entry.geometry->model);
        runtime->meshes.push_back(std::move(entry));
    }

    cache.insert({cacheKey, runtime});
    return Result<std::shared_ptr<CoalProfileRuntime>>::success(runtime);
}

} // namespace

CollisionBackendInfo coalMeshBackendInfo()
{
    return CollisionBackendInfo{
        CollisionBackendKind::Mesh,
        MeshCollisionBackendKind::Coal,
        "coal",
        true,
        true,
        true,
        true,
        "Coal mesh collision backend compiled and available",
    };
}

CollisionCheckResult checkCoalMeshBackend(const SerialRobotConfig& config,
                                          const MeshCollisionProfile& profile,
                                          const MeshCollisionCheckRequest& request)
{
    const ModelValidationResult modelValidation = RobotModelValidator::validateSerialRobotConfig(config);
    if (!modelValidation.ok()) {
        return failure(modelValidation.status(), modelValidation.issues.front().message);
    }

    const MeshCollisionProfileValidationResult profileValidation =
        MeshCollisionProfileValidator::validate(config, profile);
    if (!profileValidation.ok()) {
        return failure(profileValidation.status(), profileValidation.issues.front().message);
    }

    if (request.joints.size() != ForwardKinematics::movableJointCount(config)) {
        return failure(KinematicsStatus::JointDimensionMismatch,
                       "joint vector dimension does not match robot dof");
    }

    const Result<std::shared_ptr<CoalProfileRuntime>> runtime = loadProfileRuntime(profile);
    if (!runtime.ok()) {
        return failure(runtime.status, runtime.message);
    }

    const FkChain chain = ForwardKinematics::computeChain(config, request.joints);
    for (CoalMeshRuntimeEntry& mesh : runtime.value->meshes) {
        const auto linkIt = chain.linkPosesInBase.find(mesh.mesh.linkId);
        if (linkIt == chain.linkPosesInBase.end()) {
            return failure(KinematicsStatus::InvalidRobotConfig,
                           "mesh collision link pose not found in FK chain: " + mesh.mesh.linkId);
        }

        const Pose meshInBase = linkIt->second * mesh.mesh.meshToLink;
        mesh.object->setTransform(toCoalTransform(meshInBase));
        mesh.object->computeAABB();
    }

    std::set<std::string> disabledPairs;
    for (const DisabledCollisionPair& pair : profile.disabledPairs) {
        disabledPairs.insert(normalizePairKey(pair.geometryA, pair.geometryB));
    }

    CollisionCheckResult result;
    result.status = KinematicsStatus::Ok;

    for (std::size_t i = 0; i < runtime.value->meshes.size(); ++i) {
        for (std::size_t j = i + 1; j < runtime.value->meshes.size(); ++j) {
            const CoalMeshRuntimeEntry& a = runtime.value->meshes[i];
            const CoalMeshRuntimeEntry& b = runtime.value->meshes[j];

            if (a.mesh.id == b.mesh.id) {
                continue;
            }
            if (a.mesh.linkId == b.mesh.linkId) {
                continue;
            }
            if (disabledPairs.find(normalizePairKey(a.mesh.id, b.mesh.id)) != disabledPairs.end()) {
                continue;
            }

            coal::DistanceRequest distanceRequest(false, true);
            coal::DistanceResult distanceResult;
            coal::distance(a.object.get(), b.object.get(), distanceRequest, distanceResult);
            double distance = static_cast<double>(distanceResult.min_distance);

            const double pairMargin = a.mesh.margin_m + b.mesh.margin_m + request.safetyMargin_m;
            coal::CollisionRequest collisionRequest;
            collisionRequest.num_max_contacts = request.returnAllPairs ? std::size_t(16) : std::size_t(1);
            collisionRequest.enable_contact = true;
            collisionRequest.security_margin = pairMargin;
            collisionRequest.distance_upper_bound = (std::numeric_limits<coal::Scalar>::max)();

            coal::CollisionResult collisionResult;
            coal::collide(a.object.get(), b.object.get(), collisionRequest, collisionResult);
            const std::size_t contactCount = collisionResult.numContacts();
            const bool colliding = contactCount > 0 || distance <= pairMargin;

            if (colliding) {
                for (std::size_t contactIndex = 0; contactIndex < contactCount; ++contactIndex) {
                    const double contactDistance =
                        static_cast<double>(collisionResult.getContact(contactIndex).penetration_depth);
                    if (std::isfinite(contactDistance) && (contactIndex == 0 || contactDistance < distance)) {
                        distance = contactDistance;
                    }
                }
            }

            if (!std::isfinite(distance)) {
                return failure(KinematicsStatus::NumericalError,
                               "coal returned a non-finite mesh distance for pair '" +
                                   a.mesh.id + "'/'" + b.mesh.id + "'");
            }

            result.pairs.push_back(CollisionPairResult{
                a.mesh.id,
                b.mesh.id,
                a.mesh.linkId,
                b.mesh.linkId,
                colliding,
                distance,
                contactCount,
            });

            if (colliding) {
                result.hasCollision = true;
                if (!request.returnAllPairs) {
                    return result;
                }
            }
        }
    }

    return result;
}

} // namespace RobotKinematics
