#pragma once

#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Core/Result.h>
#include <RobotKinematics/Core/Units.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <Eigen/Core>

#include <QDir>
#include <QFileInfo>
#include <QString>

#include <cmath>
#include <optional>
#include <string>

namespace Robot3DVisualizer {

struct PendantPoseDisplay {
    double x_mm = 0.0;
    double y_mm = 0.0;
    double z_mm = 0.0;
    double rz_deg = 0.0;
    double ry_deg = 0.0;
    double rx_deg = 0.0;
};

inline double normalizeDegrees(double value)
{
    double normalized = std::fmod(value, 360.0);
    if (normalized <= -180.0) {
        normalized += 360.0;
    } else if (normalized > 180.0) {
        normalized -= 360.0;
    }
    return normalized;
}

inline RobotKinematics::Pose fromNachiPendantPose(double x_mm,
                                                  double y_mm,
                                                  double z_mm,
                                                  double rz_deg,
                                                  double ry_deg,
                                                  double rx_deg)
{
    return RobotKinematics::Pose::fromXYZRPY_mm_deg(x_mm, y_mm, z_mm, rx_deg, ry_deg, rz_deg);
}

inline PendantPoseDisplay toNachiPendantPose(const RobotKinematics::Pose& pose)
{
    const Eigen::Vector3d eulerZyx = pose.isometry().linear().canonicalEulerAngles(2, 1, 0);

    PendantPoseDisplay display;
    display.x_mm = RobotKinematics::units::toMm(pose.translation_m().x());
    display.y_mm = RobotKinematics::units::toMm(pose.translation_m().y());
    display.z_mm = RobotKinematics::units::toMm(pose.translation_m().z());
    display.rz_deg = normalizeDegrees(RobotKinematics::units::toDeg(eulerZyx[0]));
    display.ry_deg = normalizeDegrees(RobotKinematics::units::toDeg(eulerZyx[1]));
    display.rx_deg = normalizeDegrees(RobotKinematics::units::toDeg(eulerZyx[2]));
    return display;
}

inline Eigen::Matrix4d visualDeltaMatrixMm(const RobotKinematics::Pose& currentLinkInBase,
                                           const RobotKinematics::Pose& homeLinkInBase,
                                           const RobotKinematics::Pose& homeVisualCorrection =
                                               RobotKinematics::Pose::identity())
{
    const RobotKinematics::Pose visualPose =
        (currentLinkInBase * homeLinkInBase.inverse()) * homeVisualCorrection;
    Eigen::Matrix4d matrix = visualPose.isometry().matrix();
    matrix(0, 3) = RobotKinematics::units::toMm(matrix(0, 3));
    matrix(1, 3) = RobotKinematics::units::toMm(matrix(1, 3));
    matrix(2, 3) = RobotKinematics::units::toMm(matrix(2, 3));
    return matrix;
}

inline QString postureLabel(const RobotKinematics::PostureMetadata& posture,
                            const std::string& axis,
                            const std::optional<int>& branch,
                            const QString& emptyText = QStringLiteral("Any"))
{
    if (!branch.has_value()) {
        return emptyText;
    }

    const auto it = posture.labels.find(axis);
    if (it == posture.labels.end()) {
        return QStringLiteral("Unspecified");
    }

    return QString::fromStdString(*branch < 0 ? it->second.negative : it->second.positive);
}

// Returns the conventional simplified-profile path next to an original mesh
// profile path: foo_mesh_collision.json -> foo_mesh_collision_simplified.json.
// Preserves the original directory and extension. Empty input yields empty output.
inline QString meshProfileSimplifiedPath(const QString& originalPath)
{
    if (originalPath.isEmpty()) {
        return QString();
    }
    const QFileInfo info(originalPath);
    const QString base = info.completeBaseName();
    const QString suffix = info.suffix();
    QString simplifiedName = base + QStringLiteral("_simplified");
    if (!suffix.isEmpty()) {
        simplifiedName += QStringLiteral(".") + suffix;
    }
    return info.dir().filePath(simplifiedName);
}

inline QString statusText(RobotKinematics::KinematicsStatus status)
{
    using RobotKinematics::KinematicsStatus;

    switch (status) {
    case KinematicsStatus::Ok:
        return QStringLiteral("Ok");
    case KinematicsStatus::InvalidRobotConfig:
        return QStringLiteral("Invalid robot config");
    case KinematicsStatus::InvalidRequest:
        return QStringLiteral("Invalid request");
    case KinematicsStatus::FrameNotFound:
        return QStringLiteral("Frame not found");
    case KinematicsStatus::ToolNotFound:
        return QStringLiteral("Tool not found");
    case KinematicsStatus::JointDimensionMismatch:
        return QStringLiteral("Joint dimension mismatch");
    case KinematicsStatus::JointLimitViolation:
        return QStringLiteral("Joint limit violation");
    case KinematicsStatus::TargetUnreachable:
        return QStringLiteral("Target unreachable");
    case KinematicsStatus::Singularity:
        return QStringLiteral("Singularity");
    case KinematicsStatus::MaxIterationsReached:
        return QStringLiteral("Max iterations reached");
    case KinematicsStatus::NoConvergedSolution:
        return QStringLiteral("No converged solution");
    case KinematicsStatus::PostureConstraintUnsatisfied:
        return QStringLiteral("Posture constraint unsatisfied");
    case KinematicsStatus::UnsupportedSolver:
        return QStringLiteral("Unsupported solver");
    case KinematicsStatus::NumericalError:
        return QStringLiteral("Numerical error");
    }

    return QStringLiteral("Unknown status");
}

} // namespace Robot3DVisualizer
