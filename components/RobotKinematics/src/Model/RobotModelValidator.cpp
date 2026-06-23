#include <RobotKinematics/Model/RobotModelValidator.h>

#include <cmath>
#include <set>

namespace RobotKinematics {

namespace {
constexpr double kAxisNormTolerance = 1e-9;
constexpr double kLimitTolerance = 1e-12;

bool isFinite(double value)
{
    return std::isfinite(value);
}

bool containsLink(const std::set<std::string>& linkIds, const std::string& id)
{
    return !id.empty() && linkIds.find(id) != linkIds.end();
}

int movableJointCount(const SerialRobotConfig& config)
{
    int count = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            ++count;
        }
    }
    return count;
}

void addIssue(ModelValidationResult& result,
              const std::string& field,
              const std::string& message,
              KinematicsStatus status = KinematicsStatus::InvalidRobotConfig)
{
    result.issues.push_back(ModelValidationIssue{status, field, message});
}
}

bool ModelValidationResult::ok() const
{
    return issues.empty();
}

KinematicsStatus ModelValidationResult::status() const
{
    return ok() ? KinematicsStatus::Ok : issues.front().status;
}

ModelValidationResult RobotModelValidator::validateSerialRobotConfig(const SerialRobotConfig& config)
{
    ModelValidationResult result;

    if (config.topology != RobotTopologyType::Serial) {
        addIssue(result, "topology", "serial validator requires serial topology");
    }

    if (config.dof <= 0) {
        addIssue(result, "dof", "serial robot dof must be positive");
    }

    if (config.joints.empty()) {
        addIssue(result, "joints", "serial robot must contain ordered joints");
    }

    if (config.dof > 0 && movableJointCount(config) != config.dof) {
        addIssue(result, "joints", "movable joint count must match configured dof");
    }

    std::set<std::string> linkIds;
    for (std::size_t i = 0; i < config.links.size(); ++i) {
        const Link& link = config.links[i];
        const std::string field = "links[" + std::to_string(i) + "].id";
        if (link.id.empty()) {
            addIssue(result, field, "link id must not be empty");
            continue;
        }
        if (!linkIds.insert(link.id).second) {
            addIssue(result, field, "link id must be unique");
        }
    }

    if (config.frames.baseLinkId.empty()) {
        addIssue(result, "frames.base", "base link id must be set");
    } else if (!containsLink(linkIds, config.frames.baseLinkId)) {
        addIssue(result, "frames.base", "base link id must refer to a declared link");
    }

    if (config.frames.flangeLinkId.empty()) {
        addIssue(result, "frames.flange", "flange link id must be set");
    } else if (!containsLink(linkIds, config.frames.flangeLinkId)) {
        addIssue(result, "frames.flange", "flange link id must refer to a declared link");
    }

    std::set<std::string> jointIds;
    for (std::size_t i = 0; i < config.joints.size(); ++i) {
        const Joint& joint = config.joints[i];
        const std::string prefix = "joints[" + std::to_string(i) + "]";

        if (joint.id.empty()) {
            addIssue(result, prefix + ".id", "joint id must not be empty");
        } else if (!jointIds.insert(joint.id).second) {
            addIssue(result, prefix + ".id", "joint id must be unique");
        }

        if (!containsLink(linkIds, joint.parentLinkId)) {
            addIssue(result, prefix + ".parent", "joint parent must refer to a declared link");
        }
        if (!containsLink(linkIds, joint.childLinkId)) {
            addIssue(result, prefix + ".child", "joint child must refer to a declared link");
        }

        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            const double axisNorm = joint.axis.norm();
            if (!isFinite(joint.axis.x()) || !isFinite(joint.axis.y()) || !isFinite(joint.axis.z())) {
                addIssue(result, prefix + ".axis", "joint axis values must be finite");
            } else if (axisNorm <= kAxisNormTolerance) {
                addIssue(result, prefix + ".axis", "joint axis must not be zero");
            } else if (std::abs(axisNorm - 1.0) > kAxisNormTolerance) {
                addIssue(result, prefix + ".axis", "joint axis must be unit length");
            }

            if (!joint.limits.has_value()) {
                addIssue(result, prefix + ".limits", "moving joint must define position limits");
            } else {
                const JointLimits& limits = *joint.limits;
                if (!isFinite(limits.lower) || !isFinite(limits.upper)) {
                    addIssue(result, prefix + ".limits", "joint limits must be finite");
                } else if (limits.lower >= limits.upper) {
                    addIssue(result, prefix + ".limits", "joint lower limit must be less than upper limit");
                } else if (joint.home < limits.lower - kLimitTolerance || joint.home > limits.upper + kLimitTolerance) {
                    addIssue(result, prefix + ".home", "joint home must be inside position limits");
                }

                if (limits.velocity.has_value() && (!isFinite(*limits.velocity) || *limits.velocity <= 0.0)) {
                    addIssue(result, prefix + ".limits.velocity", "joint velocity limit must be positive when set");
                }
                if (limits.acceleration.has_value() && (!isFinite(*limits.acceleration) || *limits.acceleration <= 0.0)) {
                    addIssue(result, prefix + ".limits.acceleration", "joint acceleration limit must be positive when set");
                }
            }
        }
    }

    if (!config.joints.empty() && !config.frames.baseLinkId.empty() && !config.frames.flangeLinkId.empty()) {
        if (config.joints.front().parentLinkId != config.frames.baseLinkId) {
            addIssue(result, "joints[0].parent", "first serial joint parent must be the base link");
        }
        for (std::size_t i = 1; i < config.joints.size(); ++i) {
            if (config.joints[i - 1].childLinkId != config.joints[i].parentLinkId) {
                addIssue(result,
                         "joints[" + std::to_string(i) + "].parent",
                         "ordered serial joints must form a contiguous chain");
            }
        }
        if (config.joints.back().childLinkId != config.frames.flangeLinkId) {
            addIssue(result, "joints.back().child", "last serial joint child must be the flange link");
        }
    }

    std::set<std::string> userFrameIds;
    for (std::size_t i = 0; i < config.frames.userFrames.size(); ++i) {
        const UserFrame& frame = config.frames.userFrames[i];
        const std::string prefix = "frames.userFrames[" + std::to_string(i) + "]";
        if (frame.id.empty()) {
            addIssue(result, prefix + ".id", "user frame id must not be empty");
        } else if (!userFrameIds.insert(frame.id).second) {
            addIssue(result, prefix + ".id", "user frame id must be unique");
        }
        if (!containsLink(linkIds, frame.parentLinkId)) {
            addIssue(result, prefix + ".parent", "user frame parent must refer to a declared link");
        }
    }

    std::set<std::string> toolIds;
    for (std::size_t i = 0; i < config.tools.size(); ++i) {
        const Tool& tool = config.tools[i];
        const std::string field = "tools[" + std::to_string(i) + "].id";
        if (tool.id.empty()) {
            addIssue(result, field, "tool id must not be empty");
        } else if (!toolIds.insert(tool.id).second) {
            addIssue(result, field, "tool id must be unique");
        }
    }

    if (!config.defaultToolId.empty() && toolIds.find(config.defaultToolId) == toolIds.end()) {
        addIssue(result, "defaultTool", "default tool must refer to a declared tool", KinematicsStatus::ToolNotFound);
    }

    return result;
}

} // namespace RobotKinematics
