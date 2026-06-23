#pragma once

#include <RobotKinematics/Solvers/IKSolver.h>

namespace RobotKinematics {

struct NumericalIKDefaults {
    int maxIterations = 200;
    int maxSeeds = 32;

    double positionTolerance_m = 1e-6;
    double orientationTolerance_rad = 1.7453292519943296e-5;

    double stepTolerance = 1e-10;
    double costTolerance = 1e-12;

    double initialDamping = 1e-3;
    double minDamping = 1e-8;
    double maxDamping = 1e2;
    double dampingDecreaseFactor = 0.5;
    double dampingIncreaseFactor = 10.0;

    double maxJointStepNorm = 0.2;
    double jointLimitAvoidanceWeight = 0.05;
    double positionResidualWeight = 1.0;
    double orientationResidualWeight = 1.0;

    double duplicateJointTolerance = 1e-6;
};

class NumericalIKSolver : public IKSolver
{
public:
    explicit NumericalIKSolver(NumericalIKDefaults defaults = {});

    const char* name() const override;

    IKResult solve(const SerialRobotConfig& config, const IKSolveContext& context) const override;
    IKResult solveAll(const SerialRobotConfig& config, const IKSolveContext& context) const override;

private:
    NumericalIKDefaults defaults_;
};

} // namespace RobotKinematics
