# Conventions and Gotchas

Read this once. Most integration bugs come from one of these.

## Units: meters and radians, always

Core values are **SI**: meters for length, radians for angle. There is **no implicit unit
conversion** anywhere in the API. Conversions happen only at explicit, unit-named boundaries:

- `units::mm(v)`, `units::deg(v)` → SI; `units::toMm(v)`, `units::toDeg(v)` → display units.
- `JointVector::fromDegrees({...})` / `toDegrees()`.
- `Pose::fromXYZRPY_mm_deg(...)` and `translation_m()`.

If a number looks 1000× wrong, you mixed mm and m. (See [ADR-0002](decisions/adr-0002-si-units-no-implicit-conversion.md).)

## Orientation: the RPY/Euler convention

`Pose::fromXYZRPY_m_rad(x, y, z, roll, pitch, yaw)` takes the angles in **roll, pitch, yaw**
order and builds the rotation:

```
R = Rz(yaw) · Ry(pitch) · Rx(roll)
```

i.e. fixed-axis X-Y-Z (equivalently intrinsic Z-Y'-X''). `roll` is about X, `pitch` about Y,
`yaw` about Z. (See [ADR-0003](decisions/adr-0003-pose-rpy-convention.md).)

### Gotcha: vendor pendants may list the angles Z-first

A robot controller's "RX, RY, RZ" readout is **not guaranteed** to be `(roll, pitch, yaw)` in
that order. The Nachi MZ04D teach pendant, for example, lists orientation **Z-first** —
`(yaw_Z, pitch_Y, roll_X)`. To build the right `Pose` from such a triple `(o1, o2, o3)`:

```cpp
// pendant reports (o1=yaw_Z, o2=pitch_Y, o3=roll_X)
const Pose p = Pose::fromXYZRPY_mm_deg(x, y, z, /*roll=*/o3, /*pitch=*/o2, /*yaw=*/o1);
```

Getting this wrong is insidious: **position will match while orientation silently won't**.
When you onboard a new robot from pendant data, verify the orientation mapping against a few
measured poses before trusting it. See
[../preset_references/nachi-mz04d.md](../preset_references/nachi-mz04d.md) for a worked example.

## The canonical model is the source of truth

FK and IK operate **only** on `SerialRobotConfig` (URDF-like links/joints/origins/axes). DH and
URDF are **adapters** that produce or consume that model — they are not the internal
representation. If you need a robot in the system, convert it to a `SerialRobotConfig` once
(preset, builder, DH, or URDF import) and use that everywhere. (See
[ADR-0001](decisions/adr-0001-canonical-model-source-of-truth.md).)

## Joint vectors, fixed joints, and `dof`

- A `JointVector` must have exactly `ForwardKinematics::movableJointCount(config)` entries
  (revolute + prismatic only).
- A config may contain **fixed** joints (the DH adapter emits them). `SerialRobotConfig::dof`
  and validation count **movable** joints, not total joints.
- Pass joints in chain order. There is no name-based joint addressing in the FK/IK calls.

## `solve` vs `solveAll`, and what "all" means

- `solve` returns the single best solution by policy (closest to seed/previous when provided).
- `solveAll` returns every solution the **selected solver** found:
  - **Analytic solver** (supported spherical-wrist arms): exhaustive discrete branches (up to 8).
  - **Numerical solver** (everything else): *found solutions from deterministic seeds* — **not**
    a mathematically exhaustive set. Do not assume it returns every branch.
- Always check `IKResult::ok()` before `best()`; `ok()` means `status == Ok` **and** at least one
  solution exists.

## IK reference frames must be fixed to the base

`IKRequest::referenceFrame` may be empty/`"base"` (base frame) or a user frame **fixed to the
base link**. A user frame attached to a moving link returns `InvalidRequest`, because its
base-relative transform depends on the joints you are trying to solve for.

## `targetPose` is the TCP pose

`IKRequest::targetPose` is the pose of the **selected tool's TCP** (default tool if
`request.tool` is empty), expressed in `referenceFrame`. It is not the flange pose unless the
active tool is identity. For FK, `flangePose` gives the flange and `toolPose(..., flangeToTcp)`
gives a TCP.

## Errors are values, not exceptions

Fallible operations return `Result<T>` or `IKResult` carrying a `KinematicsStatus`. The library
does not throw for these. Check `.ok()` and read `.status` / `.message` on failure.

## Determinism and threading

- FK functions are pure (static, no shared state).
- `SerialRobotKinematics` holds an immutable copy of the config and its `solve`/`solveAll` are
  `const` with no shared mutable state, so calling them concurrently from multiple threads on the
  same instance is safe. Solver behavior is deterministic for a given request (fixed seed order).
- A `JointVector`/`Pose` you pass in is copied as needed; results are returned by value.

## Accuracy expectations

This is a kinematics math backend; it makes **no physical-accuracy claim** about any real robot.
Preset accuracy is only as good as the source data. The `NachiMZ04D` preset reproduces its
reference measurements to ≈0.04 mm / 0.01°, but that is a fit to teach-pendant data, not a
calibration guarantee. Numerical IK accuracy targets are `≤ 1e-6 m` position and
`≤ 1.7453e-5 rad` (~0.001°) orientation for normal, non-singular configurations.

Joint-vector round-trip checks are stricter than pose orientation residual checks: use
`≤ 1.7453292519943296e-6 rad` (0.0001°) per revolute joint by default when asserting an
expected joint vector. If the fixture's expected pose/coordinate source comes from
teach-pendant data rounded to 2 decimal places, the joint comparison may relax to
`≤ 1.7453292519943296e-5 rad` (0.001°), and the test should say so next to the assertion.

## Not in scope

No UI, path planning, dynamics, or trajectory generation. Primitive self-collision is available as a
lightweight approximate/debug path; accurate STL-backed mesh collision is planned as an optional
backend. Only serial 6DOF is implemented (no SCARA/delta/parallel). Modified (Craig) DH is not
implemented.
