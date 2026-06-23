#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Core/Units.h>

namespace RobotKinematics {

Pose::Pose()
    : transform_(Eigen::Isometry3d::Identity())
{
}

Pose::Pose(const Eigen::Isometry3d& transform)
    : transform_(transform)
{
}

Pose Pose::identity()
{
    return Pose(Eigen::Isometry3d::Identity());
}

Pose Pose::fromIsometry(const Eigen::Isometry3d& transform)
{
    return Pose(transform);
}

Pose Pose::fromXYZRPY_m_rad(double x_m, double y_m, double z_m,
                            double roll_rad, double pitch_rad, double yaw_rad)
{
    Eigen::Isometry3d transform = Eigen::Isometry3d::Identity();
    transform.translation() = Eigen::Vector3d(x_m, y_m, z_m);
    transform.linear() =
        (Eigen::AngleAxisd(yaw_rad, Eigen::Vector3d::UnitZ())
         * Eigen::AngleAxisd(pitch_rad, Eigen::Vector3d::UnitY())
         * Eigen::AngleAxisd(roll_rad, Eigen::Vector3d::UnitX()))
            .toRotationMatrix();
    return Pose(transform);
}

Pose Pose::fromXYZRPY_mm_deg(double x_mm, double y_mm, double z_mm,
                             double roll_deg, double pitch_deg, double yaw_deg)
{
    return fromXYZRPY_m_rad(units::mm(x_mm),
                            units::mm(y_mm),
                            units::mm(z_mm),
                            units::deg(roll_deg),
                            units::deg(pitch_deg),
                            units::deg(yaw_deg));
}

const Eigen::Isometry3d& Pose::isometry() const
{
    return transform_;
}

Eigen::Vector3d Pose::translation_m() const
{
    return transform_.translation();
}

Eigen::Quaterniond Pose::rotationQuaternion() const
{
    return Eigen::Quaterniond(transform_.rotation());
}

Pose Pose::inverse() const
{
    return Pose(transform_.inverse());
}

Pose Pose::operator*(const Pose& other) const
{
    return Pose(transform_ * other.transform_);
}

} // namespace RobotKinematics
