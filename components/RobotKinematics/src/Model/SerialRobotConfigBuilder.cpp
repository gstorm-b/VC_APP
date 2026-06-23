#include <RobotKinematics/Model/SerialRobotConfigBuilder.h>

#include <RobotKinematics/Model/RobotModelValidator.h>

#include <utility>

namespace RobotKinematics {

SerialRobotConfigBuilder::SerialRobotConfigBuilder()
{
    config_.topology = RobotTopologyType::Serial;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::identity(RobotIdentity value)
{
    config_.identity = std::move(value);
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::baseAndFlange(std::string baseLinkId, std::string flangeLinkId)
{
    config_.frames.baseLinkId = std::move(baseLinkId);
    config_.frames.flangeLinkId = std::move(flangeLinkId);
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::addLink(std::string id)
{
    config_.links.push_back(Link{std::move(id)});
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::addJoint(Joint joint)
{
    config_.joints.push_back(std::move(joint));
    config_.dof = static_cast<int>(config_.joints.size());
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::addUserFrame(UserFrame frame)
{
    config_.frames.userFrames.push_back(std::move(frame));
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::addTool(Tool tool)
{
    config_.tools.push_back(std::move(tool));
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::defaultTool(std::string id)
{
    config_.defaultToolId = std::move(id);
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::posture(PostureMetadata metadata)
{
    config_.posture = std::move(metadata);
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::solver(SolverMetadata metadata)
{
    config_.solver = std::move(metadata);
    return *this;
}

SerialRobotConfigBuilder& SerialRobotConfigBuilder::addSource(SourceReference source)
{
    config_.sources.push_back(std::move(source));
    return *this;
}

Result<SerialRobotConfig> SerialRobotConfigBuilder::build() const
{
    const ModelValidationResult validation = RobotModelValidator::validateSerialRobotConfig(config_);
    if (!validation.ok()) {
        return Result<SerialRobotConfig>::failure(validation.status(), validation.issues.front().message);
    }
    return Result<SerialRobotConfig>::success(config_);
}

} // namespace RobotKinematics
