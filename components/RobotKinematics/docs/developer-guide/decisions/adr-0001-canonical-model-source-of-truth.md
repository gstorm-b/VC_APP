# ADR-0001: Canonical URDF-like model is the single source of truth

## Status
Accepted

## Date
2026-06-19

## Context
A robot model can be expressed many ways: Denavit–Hartenberg (standard or modified) tables,
URDF, vendor-specific formats, or a hand-built link/joint tree. Forward and inverse kinematics
need *one* internal representation to operate on. Letting solvers understand multiple formats
would multiply complexity and create subtle inconsistencies (e.g. a DH frame and a URDF frame
that disagree).

## Decision
Define one canonical model — `SerialRobotConfig` — that is URDF-like: links, joints, per-joint
origin transforms (`Pose`), joint axes, joint limits, base/flange frames, user frames, tools,
and posture/solver metadata. **FK and IK operate only on this model.** DH and URDF are
**adapters** that convert *to* the canonical model (`DhAdapter`, `UrdfAdapter::importSerialRobot`)
or *from* it (`UrdfAdapter::exportSerialRobot`).

## Alternatives Considered
- **DH as the core representation.** Compact for arms, but awkward for tools/frames/branch
  metadata and poor for non-DH-friendly geometry. Rejected: too narrow for a general model.
- **URDF as the core representation.** Rich, but XML-centric, can't carry all our metadata
  (posture rules, solver hints, source references), and forces an XML dependency into the math
  core. Rejected: keep URDF as an adapter and warn on metadata loss.
- **Support several core formats.** Rejected: N formats × M algorithms is a maintenance and
  correctness hazard.

## Consequences
- Consumers convert any input to a `SerialRobotConfig` once (preset, builder, DH, or URDF) and
  use it everywhere.
- Adapters can be lossy in one direction (URDF can't represent posture/solver metadata); that
  loss is surfaced, not silent.
- New input formats are additive — write an adapter, don't touch the solvers.
- The model can include `fixed` joints (the DH adapter uses them); `dof`/validation count
  movable joints only.
