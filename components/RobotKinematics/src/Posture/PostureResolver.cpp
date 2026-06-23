#include <RobotKinematics/Posture/PostureResolver.h>

namespace RobotKinematics {

namespace {
int branchSign(double value)
{
    return value < 0.0 ? -1 : 1;
}

bool applyLabel(const PostureMetadata& metadata,
                const std::map<std::string, std::string>& labels,
                const std::string& axis,
                std::optional<int>& target)
{
    const auto requested = labels.find(axis);
    if (requested == labels.end()) {
        return true;
    }
    const auto configured = metadata.labels.find(axis);
    if (configured == metadata.labels.end()) {
        return false;
    }
    if (requested->second == configured->second.negative) {
        target = -1;
        return true;
    }
    if (requested->second == configured->second.positive) {
        target = 1;
        return true;
    }
    return false;
}
}

Result<ArmPosture> Serial6DofSignPostureResolver::classify(const SerialRobotConfig&,
                                                           const JointVector& joints) const
{
    if (joints.size() < 6) {
        return Result<ArmPosture>::failure(KinematicsStatus::JointDimensionMismatch,
                                           "serial 6dof posture classification requires at least 6 joints");
    }

    ArmPosture posture;
    posture.shoulder = branchSign(joints[0]);
    posture.elbow = branchSign(joints[2]);
    posture.wrist = branchSign(joints[4]);
    return Result<ArmPosture>::success(posture);
}

Result<ArmPosture> Serial6DofSignPostureResolver::fromLabels(
    const PostureMetadata& metadata,
    const std::map<std::string, std::string>& labels) const
{
    ArmPosture posture;
    if (!applyLabel(metadata, labels, "shoulder", posture.shoulder)
        || !applyLabel(metadata, labels, "elbow", posture.elbow)
        || !applyLabel(metadata, labels, "wrist", posture.wrist)) {
        return Result<ArmPosture>::failure(KinematicsStatus::PostureConstraintUnsatisfied,
                                           "posture label is not configured");
    }
    return Result<ArmPosture>::success(posture);
}

std::unique_ptr<PostureResolver> PostureResolverFactory::create(const SerialRobotConfig& config)
{
    if (config.posture.resolver == "serial_6dof_shoulder_elbow_wrist") {
        return std::make_unique<Serial6DofSignPostureResolver>();
    }
    return nullptr;
}

} // namespace RobotKinematics
