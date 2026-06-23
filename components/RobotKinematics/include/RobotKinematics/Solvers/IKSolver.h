#pragma once

#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Kinematics/InverseKinematics.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

namespace RobotKinematics {

// A request resolved by the kinematics facade into a base-frame flange goal, so solvers
// stay independent of frame/tool resolution. Solvers match FK(flange) to targetFlangeInBase.
struct IKSolveContext {
    Pose targetFlangeInBase;
    IKRequest request;
};

// Internal solver interface. Implementations: NumericalIKSolver (Phase 3), analytic
// plugins (later). Solvers operate only on the canonical SerialRobotConfig.
class IKSolver {
public:
    virtual ~IKSolver() = default;

    virtual const char* name() const = 0;

    // Returns the single best solution by policy (0 or 1 solution).
    virtual IKResult solve(const SerialRobotConfig& config, const IKSolveContext& context) const = 0;

    // Returns all solutions found by this solver from the configured seeds. For numerical
    // solvers this is found-solutions, not a mathematically exhaustive enumeration.
    virtual IKResult solveAll(const SerialRobotConfig& config, const IKSolveContext& context) const = 0;
};

} // namespace RobotKinematics
