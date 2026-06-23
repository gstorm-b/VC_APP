# Agent Instructions: RobotKinematics

This file is for AI/code agents implementing work in this repository.

## Required Context Load

Before editing code or docs, read these files:

1. `README.md`
2. `docs/robot_kinematics_spec.md`
3. `docs/planning/robot_kinematics_implementation_plan.md`
4. `docs/robot_preset_json_schema.md`
5. `docs/planning/collision_detection_plan.md` when working on collision detection
6. `docs/planning/mesh_collision_backend_plan.md` when working on mesh collision
7. `docs/developer_onboarding.md`

Do not infer missing project decisions from general robotics conventions when the spec already defines a decision.

## Current Project State

The base serial 6DOF implementation is largely complete. The repository has a Qt/qmake static library, Qt Test runner, units, `Pose`, canonical serial model, validation, joint limits, frame/tool registries, FK, numerical IK, posture-aware ranking, JSON preset loading, `Virtual6DofTestArm`, Nachi MZ04D, DH/URDF adapters, and a hybrid analytic IK plugin for supported spherical-wrist 6R robots.

Primitive self-collision detection is implemented as a fast approximate/debug path. Phase 10 mesh collision includes backend-neutral public types, mesh-profile load/validation, STL normalization (with degenerate-triangle filtering on the production path), a default unsupported mesh-backend path, an optional Coal adapter behind qmake flags, the Nachi MZ04D original mesh profile, and offline mesh simplification tooling. The Nachi MZ04D preset folder `presets/Nachi/MZ04` owns the JSON preset, STL assets, primitive collision profile, and mesh collision profile. An offline voxel-grid mesh-simplification tool lives at `tools/mesh_simplification/`; generated simplified profiles and STLs under `presets/Nachi/MZ04/simplified/` and `presets/Nachi/MZ04/*_simplified*.json` are gitignored. Core/library builds live under root `build/`; example builds live under each example folder's `build/`. The Robot3DVizualize example exposes a Collision Backend selector (Primitive / Mesh - Original / Mesh - Simplified) and can be built against the Coal-enabled library via `scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat`. RobotKinematics owns the public API, and third-party mesh collision libraries stay behind internal adapters per `docs/planning/mesh_collision_backend_plan.md`.

For mesh profiles, relative STL paths loaded with `MeshCollisionProfileJsonLoader::loadFile(path)` resolve against the mesh-profile JSON directory. `loadJson(json)` preserves authored paths. Mesh numeric fields must be JSON numbers, not strings. Optional mesh-backend dependency sources, build trees, and install outputs are intentionally kept under `third_party/` for Windows 11 + MSVC reproducibility.

Task 6.1, Kawasaki RS007N, remains blocked until the user provides verified dimensions, joint limits, posture rules, and source references. Do not invent Kawasaki data.

## Implementation Order

Follow `docs/planning/robot_kinematics_implementation_plan.md` in dependency order:

1. Phase 0: Qt/qmake project foundation.
2. Phase 1: units, pose, canonical model, validation.
3. Phase 2: joint vectors, joint limits, frames, tools, FK.
4. Phase 3: IK API and numerical IK.
5. Phase 4: posture and solution ranking.
6. Phase 5: preset JSON loader and `Virtual6DofTestArm`.
7. Phase 6: real presets; Nachi MZ04D is implemented, Kawasaki RS007N is blocked on source data.
8. Phase 7: DH/URDF adapters.
9. Phase 8: analytic IK plugin.
10. Phase 9: primitive self-collision detection.
11. Phase 10: mesh collision backend.

Do not expand into new robot families or Kawasaki RS007N before the required source data exists unless the user explicitly changes scope.

## Hard Invariants

- Core calculations use meter and radian.
- No implicit unit conversion.
- Public helper names must include units when accepting raw numeric values.
- Core pose representation is `Pose`, backed by `Eigen::Isometry3d`.
- RPY/Euler is an IO/helper format only.
- Canonical model is URDF-like and solver-facing.
- URDF and DH are adapters, not the canonical source of truth.
- Numerical `solveAll` returns solutions found by the selected solver, not mathematically exhaustive solutions.
- Real robot preset source references must be preserved.
- Preset data must not be mixed into solver logic.
- Primitive collision remains available as fallback/debug.
- Accurate mesh collision must keep third-party backend types out of public headers.
- STL-derived mesh collision assets must declare source units, scale to meters, and explicit `meshToLink` transforms.

## Required Status Enum

Use the status contract from the spec:

```cpp
enum class KinematicsStatus {
    Ok,
    InvalidRobotConfig,
    InvalidRequest,
    FrameNotFound,
    ToolNotFound,
    JointDimensionMismatch,
    JointLimitViolation,
    TargetUnreachable,
    Singularity,
    MaxIterationsReached,
    NoConvergedSolution,
    PostureConstraintUnsatisfied,
    UnsupportedSolver,
    NumericalError
};
```

## Testing Expectations

Use Qt Test.

Every implementation task must include relevant tests. At minimum:

- Unit conversion helpers.
- Pose construction and transform composition.
- Model validation.
- Joint limit validation.
- FK transform chaining.
- Frame/tool composition.
- IK API/status behavior.
- Numerical IK residuals on normal non-singular fixtures.
- Solution ranking.
- Posture resolver behavior.
- Preset JSON loading and C++ fallback equivalence.
- Collision profile validation and primitive distance checks when working on Phase 9.
- Mesh profile validation, STL unit scaling, backend availability, and mesh collision fixtures when working on Phase 10.

Accuracy targets for normal non-singular fixtures:

- Position residual: `<= 1e-6 m`.
- Orientation residual: `<= 1.7453292519943296e-5 rad` (`0.001 degree`).
- Joint-vector round-trip comparison, when comparing expected revolute joints: `<= 1.7453292519943296e-6 rad` (`0.0001 degree`) per joint by default.
- Teach-pendant/reference fixtures whose expected coordinates are rounded to 2 decimal places may use `<= 1.7453292519943296e-5 rad` (`0.001 degree`) per joint, but the test must document the source precision and why the relaxed tolerance is used.

Do not apply those tolerances to singularities, joint-limit boundaries, or intentionally unreachable targets unless the test is specifically about failure handling.

## Agent Workflow

For each task:

1. Read the relevant spec and plan section.
2. Identify the smallest task slice.
3. Add or update tests first when practical.
4. Implement only what the task requires.
5. Run the narrowest relevant tests.
6. Run the full available test command before handing off if the project can build.
7. Update docs if behavior, API, defaults, schema, or scope changes.

Prefer small, reviewable changes. Do not bundle unrelated phases together.

## Ask Before Changing

Ask the user before:

- Changing unit conventions.
- Changing public API names from the spec.
- Adding dependencies beyond Eigen, Qt, and the selected test framework.
- Adding runtime mesh collision or VTK to the core library.
- Claiming physical robot accuracy.
- Implementing SCARA, delta, parallel, 4DOF, or 5DOF support.
- Making URDF the canonical model.
- Moving analytic IK into the first milestone.
- Implementing Kawasaki data without source references.

## Good Follow-Up Tasks

If starting from the current implementation, first run the current build/test scripts and check `docs/planning/robot_kinematics_implementation_plan.md` for remaining unchecked items. If no code exists in another checkout, start with:

1. Scaffold `RobotKinematics.pro` and test target.
2. Add `include/`, `src/`, `tests/`, and `presets/` directories.
3. Implement `Units`.
4. Implement `Pose`.
5. Implement canonical model value types.

## Handoff Notes

When handing off work, include:

- Task number from the implementation plan.
- Files changed.
- Tests run.
- Known limitations.
- Any doc updates made.
- Any decisions still needing user input.
