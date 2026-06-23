#pragma once

#include <RobotKinematics/Core/Units.h>

#include <Eigen/Core>

#include <cstddef>
#include <initializer_list>
#include <utility>
#include <vector>

namespace RobotKinematics {

// Ordered joint values for a serial chain, stored in SI units: radian for revolute
// joints and meter for prismatic joints. One entry per movable joint, in chain order.
class JointVector {
public:
    JointVector() = default;
    explicit JointVector(Eigen::VectorXd values) : values_(std::move(values)) {}

    static JointVector fromRadians(const std::vector<double>& values_rad)
    {
        return JointVector(toEigen(values_rad));
    }
    static JointVector fromRadians(std::initializer_list<double> values_rad)
    {
        return fromRadians(std::vector<double>(values_rad));
    }
    // Convenience for angular (revolute) joints. Values are converted deg -> rad.
    static JointVector fromDegrees(const std::vector<double>& values_deg)
    {
        std::vector<double> rad(values_deg.size());
        for (std::size_t i = 0; i < values_deg.size(); ++i) {
            rad[i] = units::deg(values_deg[i]);
        }
        return fromRadians(rad);
    }
    static JointVector fromDegrees(std::initializer_list<double> values_deg)
    {
        return fromDegrees(std::vector<double>(values_deg));
    }

    int size() const { return static_cast<int>(values_.size()); }
    bool isEmpty() const { return values_.size() == 0; }

    double operator[](int index) const { return values_[index]; }
    double& operator[](int index) { return values_[index]; }

    const Eigen::VectorXd& values() const { return values_; } // SI units
    Eigen::VectorXd& values() { return values_; }

    std::vector<double> toDegrees() const
    {
        std::vector<double> deg(static_cast<std::size_t>(values_.size()));
        for (Eigen::Index i = 0; i < values_.size(); ++i) {
            deg[static_cast<std::size_t>(i)] = units::toDeg(values_[i]);
        }
        return deg;
    }

private:
    static Eigen::VectorXd toEigen(const std::vector<double>& v)
    {
        Eigen::VectorXd e(static_cast<Eigen::Index>(v.size()));
        for (std::size_t i = 0; i < v.size(); ++i) {
            e[static_cast<Eigen::Index>(i)] = v[i];
        }
        return e;
    }

    Eigen::VectorXd values_;
};

} // namespace RobotKinematics
