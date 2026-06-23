#pragma once

#include <RobotKinematics/Kinematics/InverseKinematics.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

namespace RobotKinematics {

class SerialRobotKinematics
{
public:
    explicit SerialRobotKinematics(SerialRobotConfig config);

    const SerialRobotConfig& config() const;

    IKResult solve(const IKRequest& request) const;
    IKResult solveAll(const IKRequest& request) const;

private:
    IKResult run(const IKRequest& request, bool allSolutions) const;

    SerialRobotConfig config_;
};

} // namespace RobotKinematics
