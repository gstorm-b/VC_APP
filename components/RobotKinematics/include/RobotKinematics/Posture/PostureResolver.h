#pragma once

#include <RobotKinematics/Core/JointVector.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>
#include <RobotKinematics/Posture/ArmPosture.h>

#include <map>
#include <memory>
#include <string>

namespace RobotKinematics {

class PostureResolver
{
public:
    virtual ~PostureResolver() = default;

    virtual Result<ArmPosture> classify(const SerialRobotConfig& config, const JointVector& joints) const = 0;
    virtual Result<ArmPosture> fromLabels(const PostureMetadata& metadata,
                                          const std::map<std::string, std::string>& labels) const = 0;
};

class Serial6DofSignPostureResolver : public PostureResolver
{
public:
    Result<ArmPosture> classify(const SerialRobotConfig& config, const JointVector& joints) const override;
    Result<ArmPosture> fromLabels(const PostureMetadata& metadata,
                                  const std::map<std::string, std::string>& labels) const override;
};

class PostureResolverFactory
{
public:
    static std::unique_ptr<PostureResolver> create(const SerialRobotConfig& config);
};

} // namespace RobotKinematics
