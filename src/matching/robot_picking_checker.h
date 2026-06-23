#ifndef ROBOT_PICKING_CHECKER_H
#define ROBOT_PICKING_CHECKER_H

namespace mtc {

// ---------------------------------------------------------------------------
// IRobotPickingChecker — port (dependency-inversion boundary) that lets the
// pure-vision ImageMatcher ask "can the robot actually pick this object?"
// without depending on the robot kinematics / calibration / device layers.
//
// ImageMatcher includes only this header (neutral POD + abstract interface).
// The concrete adapter (vc::model::RobotKinematicPickingChecker) lives in a
// higher layer and is the only place that pulls in RobotKinematics, the calib
// module, and the vc::device config.
// ---------------------------------------------------------------------------

// Neutral data-transfer pose: robot/world coordinates in millimetres, top-down
// pick rotation in degrees. Carries no robot-library or Qt types.
struct WorldPickPose {
    double x_mm{0.0};
    double y_mm{0.0};
    double z_mm{0.0};
    double r_deg{0.0};
};

class IRobotPickingChecker {
public:
    virtual ~IRobotPickingChecker() = default;

    // Step 1 — convert an image pixel position + angle (in the matcher's image
    // frame) to a world/robot pick pose. Returns false if no usable calibration.
    virtual bool imageToWorld(double imgX, double imgY, double imgAngleDeg,
                              WorldPickPose& out) const = 0;

    // Whether the operator enabled the (simplified-mesh) self-collision check.
    // ImageMatcher passes this as the withCollision flag to isPickable().
    virtual bool collisionCheckEnabled() const = 0;

    // Step 2 (+3) — reachability via solveAll IK at the pick pose; when
    // withCollision is set, the same IK solution is additionally run through the
    // simplified-mesh self-collision check. Returns true only when the pose is
    // reachable (and collision-free when requested).
    virtual bool isPickable(const WorldPickPose& pose, bool withCollision) const = 0;
};

} // namespace mtc

#endif // ROBOT_PICKING_CHECKER_H
