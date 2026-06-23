#pragma once

#include <RobotKinematics/Kinematics/InverseKinematics.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

namespace RobotKinematics {

class IKSolutionRanker
{
public:
    static bool postureMatches(const ArmPosture& candidate, const ArmPosture& requested);

    static IKSolution score(const SerialRobotConfig& config,
                            const IKRequest& request,
                            JointVector joints,
                            double positionError_m,
                            double orientationError_rad,
                            double jointLimitAvoidanceWeight);
};

} // namespace RobotKinematics
