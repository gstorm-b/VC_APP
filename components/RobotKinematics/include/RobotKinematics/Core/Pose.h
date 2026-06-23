#pragma once

#include <Eigen/Geometry>

namespace RobotKinematics {

class Pose {
public:
    Pose();

    static Pose identity();
    static Pose fromIsometry(const Eigen::Isometry3d& transform);

    // RPY convention: fixed-axis roll(X), pitch(Y), yaw(Z), applied as Rz * Ry * Rx.
    static Pose fromXYZRPY_m_rad(double x_m, double y_m, double z_m,
                                 double roll_rad, double pitch_rad, double yaw_rad);

    static Pose fromXYZRPY_mm_deg(double x_mm, double y_mm, double z_mm,
                                  double roll_deg, double pitch_deg, double yaw_deg);

    const Eigen::Isometry3d& isometry() const;
    Eigen::Vector3d translation_m() const;
    Eigen::Quaterniond rotationQuaternion() const;

    Pose inverse() const;
    Pose operator*(const Pose& other) const;

private:
    explicit Pose(const Eigen::Isometry3d& transform);

    Eigen::Isometry3d transform_;
};

} // namespace RobotKinematics
