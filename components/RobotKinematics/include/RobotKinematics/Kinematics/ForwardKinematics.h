#pragma once

#include <RobotKinematics/Core/JointVector.h>
#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <Eigen/Core>

#include <map>
#include <string>
#include <vector>

namespace RobotKinematics {

// Per-movable-joint geometric data expressed in the base frame. Used both for FK
// output and for Jacobian construction by IK solvers.
struct JointFrameData {
    Eigen::Vector3d axisInBase = Eigen::Vector3d::UnitZ();  // unit joint axis in base frame
    Eigen::Vector3d originInBase = Eigen::Vector3d::Zero(); // point on the joint axis in base frame
    JointType type = JointType::Revolute;
};

// Result of solving the full kinematic chain for a joint vector.
struct FkChain {
    std::vector<JointFrameData> joints;          // one entry per movable joint, in chain order
    std::map<std::string, Pose> linkPosesInBase; // base-relative pose of every link in the chain
    Pose flangeInBase = Pose::identity();        // base -> flange
};

// Forward kinematics for serial robot configs. All transforms are in SI units and use Pose.
class ForwardKinematics {
public:
    // Number of movable (revolute/prismatic) joints; fixed joints are excluded.
    static int movableJointCount(const SerialRobotConfig& config);

    // Solves the chain: link poses, per-joint axis/origin data, and the flange pose.
    // Expects joints.size() == movableJointCount(config); callers should validate first.
    static FkChain computeChain(const SerialRobotConfig& config, const JointVector& joints);

    // base -> flange.
    static Pose flangePose(const SerialRobotConfig& config, const JointVector& joints);

    // base -> tool TCP, given the active tool's flange->TCP transform.
    static Pose toolPose(const SerialRobotConfig& config, const JointVector& joints,
                         const Pose& flangeToTcp);

    // Resolve a user frame (defined relative to a parent link) to its base-relative pose.
    static Result<Pose> userFrameInBase(const FkChain& chain, const UserFrame& frame);
};

} // namespace RobotKinematics
