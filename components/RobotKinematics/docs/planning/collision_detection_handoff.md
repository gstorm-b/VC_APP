# Collision Detection Handoff

## Current Decision

Implement primitive self-collision detection as the next approved core extension.

Runtime collision checks must use lightweight primitive shapes only. STL files are not used for
runtime collision. STL can be used by an authoring helper to propose primitive geometry that a human
reviews before adding it to a collision profile.

This handoff is for Phase 9 primitive collision. For accurate STL-backed mesh collision, use
`docs/planning/mesh_collision_backend_plan.md` and `docs/planning/mesh_collision_backend_handoff.md`.

## Start Here

Read these files before implementation:

1. `README.md`
2. `docs/robot_kinematics_spec.md`
3. `docs/planning/robot_kinematics_implementation_plan.md`
4. `docs/robot_preset_json_schema.md`
5. `docs/planning/collision_detection_plan.md`
6. `docs/developer_onboarding.md`
7. `AGENTS.md`

## Current Status

Phase 9 is now implemented end-to-end:

- public collision API/data contracts;
- collision profile validation;
- FK-based primitive placement and pair filtering;
- sphere/capsule self-collision checks with structured pair diagnostics;
- conservative built-in profiles for `Virtual6DofTestArm` and `NachiMZ04D`;
- standalone `robot-kinematics-collision/v1` JSON loading;
- STL-to-primitive authoring helper for simple ASCII/binary STL draft proposals;
- Qt/VTK example integration that visualizes primitive collision results without using mesh
  intersections as collision truth.

The runtime still remains primitive-only and not safety-rated. STL remains authoring support only.

## Hard Rules

- Core runtime must not depend on VTK.
- Phase 9 primitive runtime must not use STL triangle meshes for collision checking. Phase 10 mesh
  collision is a separate backend plan.
- Use meter and radian only in core/profile data.
- Helper names must include units when they accept raw numeric values.
- Collision found at a valid joint state is not a `KinematicsStatus` failure.
- Keep collision profiles separate from `robot-kinematics-preset/v1` preset files.
- Do not claim physical safety or calibrated robot accuracy.
- Add Qt Test coverage for every public behavior.

## Recommended File Layout

```text
include/RobotKinematics/Collision/
  CollisionGeometry.h
  CollisionProfile.h
  CollisionProfileValidator.h
  CollisionChecker.h
  CollisionProfileJsonLoader.h

src/Collision/
  CollisionProfileValidator.cpp
  CollisionChecker.cpp
  CollisionProfileJsonLoader.cpp

tests/unit/
  CollisionProfileValidatorTests.*
  CollisionPrimitiveDistanceTests.*
  CollisionCheckerTests.*

tests/integration/
  CollisionProfileJsonTests.*

presets/
  virtual_6dof_test_arm_collision.json
  Nachi/MZ04/nachi_mz04d_collision.json
```

Adjust names to match existing repository conventions if needed, but keep collision code isolated
under a `Collision` module.

## Useful Follow-Ups

If collision work continues, likely next slices are:

1. Tune conservative primitive dimensions after more CAD/manual review.
2. Add more authoring helpers around primitive drafting/review workflow.
3. Consider additional primitive types only after measuring MVP ergonomics and performance.
4. Consider environment/world collision as a separate module after self-collision remains stable.

## Known Open Decisions

- Exact conservative primitive dimensions for Nachi MZ04D need manual review from the existing
  visual assets.
- Whether to add box/cylinder primitives after sphere/capsule performance is measured.
- Whether environment collision should be a later module after self-collision is stable.
