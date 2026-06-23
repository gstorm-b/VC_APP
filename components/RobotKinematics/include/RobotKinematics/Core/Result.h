#pragma once

#include <string>

namespace RobotKinematics {

enum class KinematicsStatus {
    Ok,
    InvalidRobotConfig,
    InvalidRequest,
    FrameNotFound,
    ToolNotFound,
    JointDimensionMismatch,
    JointLimitViolation,
    TargetUnreachable,
    Singularity,
    MaxIterationsReached,
    NoConvergedSolution,
    PostureConstraintUnsatisfied,
    UnsupportedSolver,
    NumericalError
};

// Lightweight status + value wrapper for lookups and operations that can fail
// with a structured KinematicsStatus instead of throwing.
template <typename T>
struct Result {
    KinematicsStatus status = KinematicsStatus::Ok;
    T value{};
    std::string message;

    bool ok() const { return status == KinematicsStatus::Ok; }

    static Result success(T v) { return Result{KinematicsStatus::Ok, std::move(v), {}}; }
    static Result failure(KinematicsStatus s, std::string msg = {})
    {
        return Result{s, T{}, std::move(msg)};
    }
};

} // namespace RobotKinematics
