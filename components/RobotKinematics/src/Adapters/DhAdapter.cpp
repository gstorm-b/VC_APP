#include <RobotKinematics/Adapters/DhAdapter.h>

#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Model/RobotModelValidator.h>

#include <Eigen/Geometry>

namespace RobotKinematics {

namespace {
Pose poseFromIsometry(const Eigen::Isometry3d& transform)
{
    return Pose::fromIsometry(transform);
}

Eigen::Isometry3d rzTz(double theta, double d)
{
    Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
    transform.linear() = Eigen::AngleAxisd(theta, Eigen::Vector3d::UnitZ()).toRotationMatrix();
    transform.translation() = Eigen::Vector3d(0.0, 0.0, d);
    return transform;
}

Eigen::Isometry3d txRx(double a, double alpha)
{
    Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
    transform.linear() = Eigen::AngleAxisd(alpha, Eigen::Vector3d::UnitX()).toRotationMatrix();
    transform.translation() = Eigen::Vector3d(a, 0.0, 0.0);
    return transform;
}
}

Result<SerialRobotConfig> DhAdapter::fromStandardDh(const RobotIdentity& identity,
                                                    const std::vector<StandardDhParameter>& rows)
{
    if (rows.empty()) {
        return Result<SerialRobotConfig>::failure(KinematicsStatus::InvalidRequest, "DH rows must not be empty");
    }

    SerialRobotConfig config;
    config.identity = identity;
    config.topology = RobotTopologyType::Serial;
    config.dof = static_cast<int>(rows.size());
    config.frames.baseLinkId = "base_link";
    config.frames.flangeLinkId = "flange";
    config.links.push_back({"base_link"});

    std::string parent = "base_link";
    for (std::size_t i = 0; i < rows.size(); ++i) {
        const StandardDhParameter& row = rows[i];
        const std::string motionLink = "dh_motion_" + std::to_string(i + 1);
        const std::string nextLink = (i + 1 == rows.size()) ? "flange" : "dh_link_" + std::to_string(i + 1);
        config.links.push_back({motionLink});
        config.links.push_back({nextLink});

        Joint joint;
        joint.id = row.jointId.empty() ? "J" + std::to_string(i + 1) : row.jointId;
        joint.type = row.type == DhJointType::Revolute ? JointType::Revolute : JointType::Prismatic;
        joint.parentLinkId = parent;
        joint.childLinkId = motionLink;
        joint.origin = poseFromIsometry(rzTz(row.type == DhJointType::Revolute ? row.thetaOffset_rad : 0.0,
                                             row.type == DhJointType::Prismatic ? 0.0 : row.d_m));
        joint.axis = Eigen::Vector3d::UnitZ();
        joint.limits = row.limits;
        joint.home = row.home;
        config.joints.push_back(joint);

        Joint fixed;
        fixed.id = joint.id + "_link";
        fixed.type = JointType::Fixed;
        fixed.parentLinkId = motionLink;
        fixed.childLinkId = nextLink;
        fixed.origin = poseFromIsometry(txRx(row.a_m, row.alpha_rad));
        if (row.type == DhJointType::Prismatic) {
            fixed.origin = poseFromIsometry(rzTz(row.thetaOffset_rad, row.d_m) * txRx(row.a_m, row.alpha_rad));
        }
        config.joints.push_back(fixed);
        parent = nextLink;
    }

    const ModelValidationResult validation = RobotModelValidator::validateSerialRobotConfig(config);
    if (!validation.ok()) {
        return Result<SerialRobotConfig>::failure(validation.status(), validation.issues.front().message);
    }
    return Result<SerialRobotConfig>::success(config);
}

} // namespace RobotKinematics
