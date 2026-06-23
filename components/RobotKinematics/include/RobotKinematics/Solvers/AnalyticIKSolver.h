#pragma once

#include <RobotKinematics/Solvers/IKSolver.h>

namespace RobotKinematics {

class AnalyticIKSolver : public IKSolver
{
public:
    virtual bool supportsModel(const SerialRobotConfig& config) const = 0;
};

} // namespace RobotKinematics
