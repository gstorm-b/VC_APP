# Milestone Close-Out

## Baseline

RobotKinematics is ready to use as a core C++17 / Eigen / QtCore kinematics backend for serial
industrial robot work. The current baseline includes:

- canonical serial robot model with meter/radian units;
- `Pose`, `JointVector`, model validation, joint limits, frames, and tools;
- FK and hybrid IK (`SerialRobotKinematics::solve` / `solveAll`);
- posture-aware solution ranking;
- JSON preset loading with `robot-kinematics-preset/v1`;
- `Virtual6DofTestArm` and `NachiMZ04D`;
- DH and URDF-like adapters;
- primitive self-collision checking;
- optional Coal-backed mesh collision with Nachi original STL profile and simplification tooling;
- no-UI console examples and a Qt/VTK visualizer example.

## Verification Commands

Default core:

```powershell
scripts\build_msvc.bat
scripts\test_msvc.bat
```

Optional mesh backend:

```powershell
scripts\build_msvc_mesh_coal.bat
scripts\test_msvc_mesh_coal.bat
```

No-UI examples:

```powershell
scripts\build_example_nachi_mz04_cli_msvc.bat
scripts\build_example_custom_preset_cli_msvc.bat
```

Visualizer:

```powershell
scripts\build_example_robot3dvisualize_msvc.bat
scripts\build_example_robot3dvisualize_mesh_coal_msvc.bat
```

## Stable Entry Points

- [README.md](../../README.md): project status and start-here map.
- [developer_onboarding.md](../developer_onboarding.md): implementation context for new devs/agents.
- [developer-guide/README.md](../developer-guide/README.md): library user guide.
- [examples/README.md](../../examples/README.md): runnable examples.
- [robot_kinematics_spec.md](../robot_kinematics_spec.md): core contract.
- [robot_preset_json_schema.md](../robot_preset_json_schema.md): preset schema.

## Known Follow-Ups

- Kawasaki RS007N remains blocked on verified source data.
- Capture a real primitive-miss / mesh-hit Nachi pose and tighten the mesh regression test.
- Add manual visual QA screenshots for Robot3DVizualize mesh modes if release evidence is needed.
- Confirm final distribution/license policy before shipping optional third-party mesh dependencies.

## Scope Boundaries For Next Expansion

Ask before adding:

- new robot families or Kawasaki data without source references;
- SCARA, delta, parallel, 4DOF, or 5DOF support;
- path planning, dynamics, environment/world collision, or swept collision;
- runtime VTK dependency in the core library;
- any claim of physical safety certification or calibrated real-robot accuracy.
