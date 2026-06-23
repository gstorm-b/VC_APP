#include <RobotKinematics/Collision/CollisionProfileValidator.h>

#include <set>

namespace RobotKinematics {

namespace {
void addIssue(CollisionProfileValidationResult& result,
              const std::string& field,
              const std::string& message,
              KinematicsStatus status = KinematicsStatus::InvalidRobotConfig)
{
    result.issues.push_back(CollisionProfileValidationIssue{status, field, message});
}

bool hasLinkId(const std::set<std::string>& linkIds, const std::string& linkId)
{
    return !linkId.empty() && linkIds.find(linkId) != linkIds.end();
}
}

bool CollisionProfileValidationResult::ok() const
{
    return issues.empty();
}

KinematicsStatus CollisionProfileValidationResult::status() const
{
    return ok() ? KinematicsStatus::Ok : issues.front().status;
}

CollisionProfileValidationResult CollisionProfileValidator::validate(const SerialRobotConfig& config,
                                                                     const CollisionProfile& profile)
{
    CollisionProfileValidationResult result;

    if (profile.id.empty()) {
        addIssue(result, "profile.id", "collision profile id must not be empty");
    }
    if (profile.robotModel.empty()) {
        addIssue(result, "profile.robotModel", "collision profile robot model must not be empty");
    }

    std::set<std::string> linkIds;
    for (const Link& link : config.links) {
        if (!link.id.empty()) {
            linkIds.insert(link.id);
        }
    }

    std::set<std::string> geometryIds;
    for (std::size_t i = 0; i < profile.geometries.size(); ++i) {
        const CollisionGeometry& geometry = profile.geometries[i];
        const std::string prefix = "geometries[" + std::to_string(i) + "]";

        if (geometry.id.empty()) {
            addIssue(result, prefix + ".id", "collision geometry id must not be empty");
        } else if (!geometryIds.insert(geometry.id).second) {
            addIssue(result, prefix + ".id", "collision geometry id must be unique");
        }

        if (!hasLinkId(linkIds, geometry.linkId)) {
            addIssue(result, prefix + ".linkId", "collision geometry link id must refer to a declared link");
        }

        if (geometry.margin_m < 0.0) {
            addIssue(result, prefix + ".margin_m", "collision geometry margin must be non-negative");
        }

        switch (geometry.shape.type) {
        case CollisionShapeType::Sphere:
            if (geometry.shape.sphere.radius_m <= 0.0) {
                addIssue(result, prefix + ".sphere.radius_m", "collision sphere radius must be positive");
            }
            break;
        case CollisionShapeType::Capsule:
            if (geometry.shape.capsule.radius_m <= 0.0) {
                addIssue(result, prefix + ".capsule.radius_m", "collision capsule radius must be positive");
            }
            if (geometry.shape.capsule.length_m <= 0.0) {
                addIssue(result, prefix + ".capsule.length_m", "collision capsule length must be positive");
            }
            break;
        }
    }

    for (std::size_t i = 0; i < profile.disabledPairs.size(); ++i) {
        const DisabledCollisionPair& pair = profile.disabledPairs[i];
        const std::string prefix = "disabledPairs[" + std::to_string(i) + "]";

        if (pair.geometryA.empty() || geometryIds.find(pair.geometryA) == geometryIds.end()) {
            addIssue(result, prefix + ".geometryA",
                     "disabled collision pair geometryA must refer to an existing geometry id");
        }
        if (pair.geometryB.empty() || geometryIds.find(pair.geometryB) == geometryIds.end()) {
            addIssue(result, prefix + ".geometryB",
                     "disabled collision pair geometryB must refer to an existing geometry id");
        }
    }

    return result;
}

} // namespace RobotKinematics
