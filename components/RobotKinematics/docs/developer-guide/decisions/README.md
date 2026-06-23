# Architecture Decision Records

These ADRs capture the *why* behind RobotKinematics' design — the decisions that are expensive
to reverse and that shape how you use the library. They record decisions already in force; they
are written here so consumers and future maintainers don't have to re-derive the reasoning.

| ADR | Decision | Status |
|---|---|---|
| [ADR-0001](adr-0001-canonical-model-source-of-truth.md) | Canonical URDF-like model is the single source of truth; DH/URDF are adapters | Accepted |
| [ADR-0002](adr-0002-si-units-no-implicit-conversion.md) | SI units (meter/radian) with no implicit conversion | Accepted |
| [ADR-0003](adr-0003-pose-rpy-convention.md) | `Pose` wraps `Eigen::Isometry3d`; RPY is `Rz(yaw)·Ry(pitch)·Rx(roll)` | Accepted |
| [ADR-0004](adr-0004-hybrid-analytic-numerical-ik.md) | Hybrid IK: analytic plugin when supported, numerical fallback | Accepted |

Process: don't delete superseded ADRs — write a new one that references and supersedes the old.
See the broader rationale in [../../robot_kinematics_spec.md](../../robot_kinematics_spec.md) and
[../../planning/robot_kinematics_implementation_plan.md](../../planning/robot_kinematics_implementation_plan.md).
