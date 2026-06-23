#pragma once

#include <RobotKinematics/Core/Ids.h>
#include <RobotKinematics/Core/JointVector.h>
#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Posture/ArmPosture.h>

#include <optional>
#include <string>
#include <vector>

namespace RobotKinematics {

// IK and validation share the canonical KinematicsStatus contract.
using IKStatus = KinematicsStatus;

struct IKOptions {
    bool returnClosestToSeed = true;
    bool requirePosture = false;
    double maxPositionError_m = 1e-6;
    // Reconciled to the spec's NumericalIKDefaults / accuracy criteria (0.001 deg). The
    // IKOptions example in the spec showed 1e-6 rad; the authoritative default is below.
    double maxOrientationError_rad = 1.7453292519943296e-5;
    int maxSolutions = 16;
};

struct IKRequest {
    Pose targetPose;                          // target TCP pose, expressed in referenceFrame
    std::optional<JointVector> seedJoint;     // preferred starting joints
    std::optional<JointVector> previousJoint; // last commanded joints (motion continuity)
    std::optional<ArmPosture> posture;        // requested/required posture branch
    FrameId referenceFrame;                   // empty => base frame
    ToolId tool;                              // empty => default tool
    IKOptions options;
};

struct IKSolutionScore {
    double totalCost = 0.0;
    double seedDistanceCost = 0.0;
    double jointLimitMarginCost = 0.0;
    double postureMismatchCost = 0.0;
    double motionContinuityCost = 0.0;
};

struct IKSolution {
    JointVector joints;
    ArmPosture posture;
    double positionError_m = 0.0;
    double orientationError_rad = 0.0;
    IKSolutionScore score;
};

struct IKResult {
    IKStatus status = IKStatus::NoConvergedSolution;
    std::vector<IKSolution> solutions;
    std::string message;

    // Ok means at least one valid solution is available.
    bool ok() const { return status == IKStatus::Ok && !solutions.empty(); }

    // Precondition: !solutions.empty(). Callers should check ok() first.
    const IKSolution& best() const { return solutions.front(); }
};

} // namespace RobotKinematics
