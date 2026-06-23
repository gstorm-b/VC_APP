#pragma once

#include <RobotKinematics/Solvers/AnalyticIKSolver.h>

namespace RobotKinematics {

class Analytic6DofSphericalWristSolver : public AnalyticIKSolver
{
public:
    const char* name() const override;
    bool supportsModel(const SerialRobotConfig& config) const override;

    IKResult solve(const SerialRobotConfig& config, const IKSolveContext& context) const override;
    IKResult solveAll(const SerialRobotConfig& config, const IKSolveContext& context) const override;
};

} // namespace RobotKinematics
