# ADR-0003: `Pose` wraps `Eigen::Isometry3d`; RPY is `Rz(yaw)·Ry(pitch)·Rx(roll)`

## Status
Accepted

## Date
2026-06-19

## Context
The library passes rigid transforms everywhere. A canonical transform type is needed, plus a
documented Euler convention for human/IO formats. Euler angles are convention-cursed: "RPY"
alone is ambiguous about axis order, intrinsic vs extrinsic, and which reported number is which
angle. An undocumented convention guarantees integration bugs.

## Decision
- The canonical transform type is `Pose`, a thin wrapper around `Eigen::Isometry3d`. Internally,
  transforms are matrices/quaternions — **never** stored as Euler angles.
- The RPY helper convention is fixed and documented: `Pose::fromXYZRPY_*(x, y, z, roll, pitch,
  yaw)` builds `R = Rz(yaw)·Ry(pitch)·Rx(roll)` (fixed-axis X-Y-Z / intrinsic Z-Y'-X''), with
  `roll` about X, `pitch` about Y, `yaw` about Z.
- This convention is covered by unit tests.

## Alternatives Considered
- **Store orientation as RPY internally.** Rejected: Euler angles have singularities (gimbal
  lock) and are convention-dependent; matrices/quaternions are the robust internal truth.
- **A different Euler order (e.g. XYZ intrinsic).** Either is defensible; we picked Z-Y-X
  fixed-axis and, crucially, **documented and tested it**. The order matters less than nailing
  it down.

## Consequences
- All orientation IO goes through the documented helpers; raw transforms are available via
  `isometry()` / `rotationQuaternion()`.
- Vendor data may use a *different* order. Notably, some teach pendants list the triple
  **Z-first** (`yaw, pitch, roll`), so callers must map fields explicitly when ingesting pendant
  poses (the symptom of getting it wrong is "position matches, orientation doesn't"). See
  `conventions-and-gotchas.md` and the Nachi MZ04D preset reference.
