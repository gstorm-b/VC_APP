#pragma once

#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

namespace RobotKinematics {

class SerialRobotConfigBuilder
{
public:
    SerialRobotConfigBuilder();

    SerialRobotConfigBuilder& identity(RobotIdentity value);
    SerialRobotConfigBuilder& baseAndFlange(std::string baseLinkId, std::string flangeLinkId);
    SerialRobotConfigBuilder& addLink(std::string id);
    SerialRobotConfigBuilder& addJoint(Joint joint);
    SerialRobotConfigBuilder& addUserFrame(UserFrame frame);
    SerialRobotConfigBuilder& addTool(Tool tool);
    SerialRobotConfigBuilder& defaultTool(std::string id);
    SerialRobotConfigBuilder& posture(PostureMetadata metadata);
    SerialRobotConfigBuilder& solver(SolverMetadata metadata);
    SerialRobotConfigBuilder& addSource(SourceReference source);

    Result<SerialRobotConfig> build() const;

private:
    SerialRobotConfig config_;
};

} // namespace RobotKinematics
