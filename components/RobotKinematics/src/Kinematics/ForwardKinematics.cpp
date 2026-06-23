#include <RobotKinematics/Kinematics/ForwardKinematics.h>

#include <Eigen/Geometry>

namespace RobotKinematics {

int ForwardKinematics::movableJointCount(const SerialRobotConfig& config)
{
    int count = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            ++count;
        }
    }
    return count;
}

FkChain ForwardKinematics::computeChain(const SerialRobotConfig& config, const JointVector& joints)
{
    FkChain chain;

    Eigen::Isometry3d accumulated = Eigen::Isometry3d::Identity(); // base link pose in base
    if (!config.frames.baseLinkId.empty()) {
        chain.linkPosesInBase[config.frames.baseLinkId] = Pose::fromIsometry(accumulated);
    }

    int movableIndex = 0;
    for (const Joint& joint : config.joints) {
        // Joint frame in base, before applying the joint's own motion.
        const Eigen::Isometry3d jointFrame = accumulated * joint.origin.isometry();

        Eigen::Isometry3d motion = Eigen::Isometry3d::Identity();
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            const double value = (movableIndex < joints.size()) ? joints[movableIndex] : 0.0;
            const Eigen::Vector3d axisLocal = joint.axis.normalized();

            JointFrameData data;
            data.type = joint.type;
            data.axisInBase = jointFrame.linear() * axisLocal;
            data.originInBase = jointFrame.translation();
            chain.joints.push_back(data);

            if (joint.type == JointType::Revolute) {
                motion.linear() = Eigen::AngleAxisd(value, axisLocal).toRotationMatrix();
            } else {
                motion.translation() = axisLocal * value;
            }
            ++movableIndex;
        }

        accumulated = jointFrame * motion; // child link pose in base
        if (!joint.childLinkId.empty()) {
            chain.linkPosesInBase[joint.childLinkId] = Pose::fromIsometry(accumulated);
        }
    }

    chain.flangeInBase = Pose::fromIsometry(accumulated);
    if (!config.frames.flangeLinkId.empty()) {
        const auto it = chain.linkPosesInBase.find(config.frames.flangeLinkId);
        if (it != chain.linkPosesInBase.end()) {
            chain.flangeInBase = it->second;
        }
    }
    return chain;
}

Pose ForwardKinematics::flangePose(const SerialRobotConfig& config, const JointVector& joints)
{
    return computeChain(config, joints).flangeInBase;
}

Pose ForwardKinematics::toolPose(const SerialRobotConfig& config, const JointVector& joints,
                                 const Pose& flangeToTcp)
{
    return flangePose(config, joints) * flangeToTcp;
}

Result<Pose> ForwardKinematics::userFrameInBase(const FkChain& chain, const UserFrame& frame)
{
    const auto it = chain.linkPosesInBase.find(frame.parentLinkId);
    if (it == chain.linkPosesInBase.end()) {
        return Result<Pose>::failure(KinematicsStatus::FrameNotFound,
                                     "user frame parent link not found: " + frame.parentLinkId);
    }
    return Result<Pose>::success(it->second * frame.transform);
}

} // namespace RobotKinematics
