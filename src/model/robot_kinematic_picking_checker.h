#ifndef ROBOT_KINEMATIC_PICKING_CHECKER_H
#define ROBOT_KINEMATIC_PICKING_CHECKER_H

#include "matching/robot_picking_checker.h"
#include "calibration/calibrator.h"
#include "device/output_device/vision_output_config.h"   // RobotKinematicCheckConfig

#include <RobotKinematics/Model/RobotModelConfig.h>       // SerialRobotConfig

#include <vector>

namespace vc::model {

// ---------------------------------------------------------------------------
// RobotKinematicPickingChecker — concrete mtc::IRobotPickingChecker.
//
// The single bridge that pulls together the three families the pure-vision
// matcher must not know about: the calib module (image -> world), the
// RobotKinematics component (solveAll IK + simplified-mesh collision), and the
// vc::device robot-check config (preset, TCP, collision toggle).
//
// Built by the layer that owns both the calibration and the robot config (the
// localization runtime), then injected into an ImageMatcher via
// setRobotPickingChecker(). The matcher only sees the mtc::IRobotPickingChecker
// interface, so it stays free of RobotKinematics / calib / vc dependencies.
// ---------------------------------------------------------------------------
class RobotKinematicPickingChecker : public mtc::IRobotPickingChecker {
public:
    RobotKinematicPickingChecker(const calib::Calibrator& calibrator,
                                 vc::device::RobotKinematicCheckConfig config);

    bool imageToWorld(double imgX, double imgY, double imgAngleDeg,
                      mtc::WorldPickPose& out) const override;
    bool collisionCheckEnabled() const override;
    bool isPickable(const mtc::WorldPickPose& pose, bool withCollision) const override;

private:
    // One picking-path waypoint, prepared from a config PickPathPoint: a 6-axis
    // tool-frame offset (mm/deg) plus required posture branch signs (-1/+1, or 0
    // for "any") resolved from the preset's posture labels at construction.
    struct Waypoint {
        double dx, dy, dz, dRoll, dPitch, dYaw;
        int shoulder, elbow, wrist;
    };

    calib::Calibrator m_calibrator;                       // copy (lifetime-safe)
    vc::device::RobotKinematicCheckConfig m_config;
    RobotKinematics::SerialRobotConfig m_robot;           // preset + picking TCP
    bool m_robotValid{false};                             // false => preset unknown
    std::vector<Waypoint> m_waypoints;                    // path (>=1 entry)
};

} // namespace vc::model

#endif // ROBOT_KINEMATIC_PICKING_CHECKER_H
