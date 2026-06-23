# Developer Onboarding: RobotKinematics

## What We Are Building

RobotKinematics is a reusable C++ backend library for industrial robot kinematics.

It lets developer engineers:

- define or load a robot model;
- compute forward kinematics;
- solve inverse kinematics;
- validate joint limits, primitive self-collision profiles, and optional accurate mesh collision profiles;
- configure base/user/tool frames;
- load custom robot presets;
- choose IK solutions using seed, previous joint state, posture, and options.

The base milestone is implemented and verified with a project-owned virtual serial 6DOF robot named
`Virtual6DofTestArm`; the current repo also includes the Nachi MZ04D preset and collision examples.

## What To Read First

Read in this order:

1. `README.md` for the quick project summary.
2. `docs/robot_kinematics_spec.md` for the contract.
3. `docs/planning/robot_kinematics_implementation_plan.md` for task order.
4. `docs/robot_preset_json_schema.md` for preset format.
5. `AGENTS.md` if you are using AI agents to implement tasks.

## Current Milestone

Base code milestone:

- Qt 6/qmake C++ library.
- Eigen math.
- Qt Test.
- MSVC primary compiler.
- MinGW compatibility compiler.
- Core model for serial 6DOF robots.
- FK, numerical IK, frames, tools, joint limits.
- Posture metadata and posture-aware IK selection.
- JSON preset loading.
- Built-in C++ fallback preset.
- `Virtual6DofTestArm` integration tests.

`NachiMZ04D` has since been implemented from teach-pendant reference data. `KawasakiRS007N` remains blocked until verified source data is provided; see `docs/preset_references/kawasaki-rs007n.md`.

Current implementation status:

- Phase 0 Qt/qmake static library scaffold exists.
- Qt Test runner exists at `tests/RobotKinematicsTests.exe` after build.
- Phase 1 core types exist for units, `Pose`, canonical serial model config, `KinematicsStatus`, and serial model validation.
- Phase 2 exists for joint vectors, joint-limit validation, frame/tool registries, and FK, and has been verified with the MSVC build/test flow.
- Phase 3 exists for IK API types, adaptive damped least-squares numerical IK, and `SerialRobotKinematics::solve/solveAll` orchestration.
- Phase 4 exists for generic posture metadata, a metadata-gated serial 6DOF posture resolver, posture-derived seed expansion, and posture-aware IK scoring/rejection.
- Phase 5 exists for custom config building, preset JSON loading, and `Virtual6DofTestArm` as both JSON and built-in C++ fallback.
- Phase 6 real presets: Nachi MZ04D is implemented from teach-pendant data; Kawasaki RS007N remains blocked until verified source data is provided.
- Phase 7 exists for standard DH import to canonical config and URDF-like export/import. Modified DH remains a later adapter extension.
- Phase 8 analytic IK capability detection and spherical-wrist analytic IK are implemented for supported morphologies.
- Phase 9 primitive self-collision detection is implemented as a fast approximate/debug path.
- Phase 10 mesh collision is implemented as an optional backend path: backend-neutral public types,
  mesh-profile load/validation, STL normalization, optional Coal adapter, Nachi original STL mesh
  profile, offline mesh simplification tooling, benchmark scripts, and visualizer backend selector
  now exist. The default build still does not compile a mesh backend unless explicit qmake flags are
  enabled.

## Key Architecture Decisions

### Canonical Model

The internal model is URDF-like:

- links;
- joints;
- joint origin transforms;
- joint axes;
- joint limits;
- base/flange frames;
- tool and user frames;
- solver metadata;
- posture metadata.

URDF and DH/Modified DH are adapter formats. They are not the internal source of truth.

### Units

Core values use SI units:

- meter for length;
- radian for angle.

Helpers may accept or return millimeter/degree, but helper names must make the unit explicit. Do not perform implicit unit conversion.

### Pose

Core pose uses a `Pose` wrapper backed by `Eigen::Isometry3d`.

RPY/Euler, quaternion, XYZABC, and XYZRPY are helper/IO formats only. RPY convention must be documented and tested.

### IK

The first numerical IK method is adaptive damped least squares.

Default targets:

- position residual `<= 1e-6 m`;
- orientation residual `<= 1.7453292519943296e-5 rad`.
- joint-vector round-trip comparison `<= 1.7453292519943296e-6 rad` (`0.0001 degree`) per revolute joint when an expected joint vector is asserted.

Tests based on teach-pendant/reference coordinates rounded to 2 decimal places may relax joint-vector comparison to `<= 1.7453292519943296e-5 rad` (`0.001 degree`) per revolute joint, but must document the source precision. This exception does not relax the TCP position/orientation residual criteria.

`solve` returns one best solution by policy.

`solveAll` returns all solutions found by the selected solver. For numerical IK, this is not a mathematical exhaustive guarantee.

### Presets

Preset files use JSON schema `robot-kinematics-preset/v1`.

The first preset is `Virtual6DofTestArm`. It exists to validate library behavior before real vendor data is available.

Nachi MZ04D is available as a real preset. Kawasaki RS007N should be added only after source dimensions, joint limits, and posture rules are provided.

### Collision

The collision module currently has a primitive-first runtime path.

- Runtime collision checks use sphere/capsule profiles.
- STL files are not used by the primitive backend.
- STL may be used by helper tooling to propose draft primitives for manual review.
- Collision profiles use a separate schema, `robot-kinematics-collision/v1`.
- Collision found at a valid joint state is represented by `hasCollision`, not a failure status.
- No physical safety or calibration accuracy claim is made.

The current Phase 10 extension keeps public APIs RobotKinematics-owned and backend-neutral. Mesh
profile loading, validation, and STL normalization are available in the default build. Real runtime
mesh collision is available only when the optional Coal adapter is compiled; otherwise mesh requests
still return a structured unsupported result. Coal/FCL/VTK/Open3D/CGAL/libigl types must not leak
into public headers. Mesh assets must declare source units, `scaleToMeters`, and explicit
`meshToLink` transforms. Relative mesh paths loaded from mesh-profile files resolve against the
profile JSON directory. Optional backend dependency sources, build trees, and install roots live
under `third_party/` so the Windows 11 + MSVC environment can be reproduced from source.

## Source Layout

```text
include/
  RobotKinematics/
    Core/
    Model/
    Kinematics/
    Solvers/
    Posture/
    Presets/
    Adapters/

src/
  Core/
  Model/
  Kinematics/
  Solvers/
  Collision/
  Posture/
  Presets/
  Adapters/

presets/
  virtual_6dof_test_arm.json

tests/
  unit/
  integration/
  fixtures/

examples/
  NachiMZ04Cli/
  CustomPresetCli/
  Robot3DVizualize/
```

## Build And Test

Expected MSVC flow:

```powershell
scripts\build_msvc.bat
scripts\test_msvc.bat
```

Clean MSVC rebuild plus tests:

```powershell
scripts\rebuild_msvc.bat
```

Expected MinGW compatibility flow:

```powershell
scripts\build_mingw.bat
scripts\test_mingw.bat
```

Optional Coal mesh-backend flow:

```powershell
scripts\build_msvc_mesh_coal.bat
scripts\test_msvc_mesh_coal.bat
scripts\build_mesh_collision_benchmark_msvc.bat
scripts\run_mesh_collision_benchmark_msvc.bat
```

Build output convention:

- core/library/test builds live under repository-root `build/` (`build/msvc`, `build/mingw`,
  `build/msvc_mesh_coal`, and `build/tools`);
- example builds live under the example folder, e.g. `examples/Robot3DVizualize/build/msvc`;
- optional third-party dependency sources, build trees, and install outputs live under
  `third_party/` (`third_party/<name>`, `third_party/build/<name>`, and
  `third_party/install/<name>`);
- Nachi MZ04D preset, STL assets, primitive profile, and mesh profile live together under
  `presets/Nachi/MZ04`.

Third-party dependency builds are currently documented and supported for Windows 11 with MSVC
through the `scripts/build_third_party_*_msvc.bat` scripts. Run them from a Windows 11 machine with
Visual Studio 2022 Build Tools/MSVC available through `VCVARS` and Qt's CMake available through
`QT_CMAKE` when a script requires CMake.

## How To Pick Up Work

Use `docs/planning/robot_kinematics_implementation_plan.md`.

Pick the first incomplete task whose dependencies are complete. Keep the change small enough to review in one focused session.

Good initial task order:

1. Scaffold qmake library and Qt Test target.
2. Implement units helpers.
3. Implement `Pose`.
4. Implement canonical model types.
5. Implement model validation.
6. Implement joint vector and joint limits.
7. Implement frame/tool registry.
8. Implement FK.
9. Implement IK API and statuses.
10. Implement numerical IK.
11. Implement posture/ranking.
12. Implement JSON loader and `Virtual6DofTestArm`.
13. Implement primitive collision detection using `docs/planning/collision_detection_plan.md`.
14. Implement mesh collision backend using `docs/planning/mesh_collision_backend_plan.md`.

## Definition Of Done

A task is done when:

- it satisfies the task acceptance criteria;
- relevant unit/integration tests exist;
- relevant tests pass locally;
- public API changes are documented;
- spec/plan/schema docs are updated if behavior changed;
- no unrelated scope was added.

The base milestone is done when `Virtual6DofTestArm` passes FK/IK/frame/tool/joint-limit/posture tests and the library builds with MSVC. This repository has passed that milestone and now carries Phase 9/10 collision extensions plus example applications.

Current IK limitation: request reference frames are supported when empty/base or when a user frame is fixed to the base link. User frames attached to moving links return `InvalidRequest` for IK because their base-relative transform depends on the unknown joint vector.

Canonical serial configs may contain fixed joints for adapter-generated link transforms. `SerialRobotConfig::dof` counts movable joints, and validation enforces movable joint count rather than total joint count.

## Common Mistakes To Avoid

- Do not use millimeter/degree internally.
- Do not store RPY as the internal pose truth.
- Do not make URDF the core model.
- Do not claim physical robot accuracy.
- Do not treat numerical `solveAll` as exhaustive.
- Do not implement Kawasaki RS007N without source references.
- Do not put robot-specific preset data inside generic solver code.
- Do not expand to SCARA, delta, parallel, or analytic IK before the base milestone.
- Do not expose third-party mesh collision library types in public APIs.
- Do not treat primitive collision profiles as safety-rated physical geometry.
