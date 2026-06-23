# Agent Instructions: Robot3DVizualize

This file is for agents implementing the Nachi MZ04D Qt/VTK example.

## Required Context Load

Before editing this example, read:

1. `README.md`
2. `IMPLEMENTATION_PLAN.md`
3. `HANDOFF.md`
4. `../../AGENTS.md`
5. `../../docs/developer-guide/conventions-and-gotchas.md`
6. `../../docs/preset_references/nachi-mz04d.md`

## Current State

Phase 6 is complete. The example:

- is isolated from the root `RobotKinematics.pro` build;
- builds with Qt 6/qmake and external VTK;
- links against the core `RobotKinematics` static library through `Robot3DVizualize.pro`;
- uses the local VTK path `D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs` as the MSVC build-script default when `VTK_ROOT` is unset;
- starts a `QVTKOpenGLNativeWidget` scene;
- loads all eight STL runtime assets from `presets/Nachi/MZ04/`;
- renders orientation axes, a simple ground plane, distinct part colors, and basic lighting;
- updates STL actors from live FK using home-relative visual transforms;
- applies a documented static correction to `Centering_tool.stl` before the flange FK delta;
- exposes persistent FK, IK, and posture controls in `mainwindow.ui`;
- exposes a Debug tab for mesh visibility, mesh origins, and FK axis inspection;
- supports `solve`, `solveAll`, apply-selected-solution, and posture preference/requirement requests;
- reports missing/unreadable STL assets through a visible warning and status-bar message.

There is no remaining planned implementation phase in `IMPLEMENTATION_PLAN.md`; follow-up work should be treated as maintenance, CAD-alignment iteration, or user-directed scope changes.

## Dependency Rules

- Do not vendor VTK into this repository.
- Do not add VTK to the core library or default root build.
- Do not replace qmake with CMake for this example unless the user explicitly approves.
- Use the existing `Robot3DVizualize.pro` as the source of truth for Qt Creator builds and tests. Do not create a parallel `.pro`, `.pri`, or CMake entrypoint that bypasses it unless the user explicitly approves.
- Keep example-specific build logic inside `examples/Robot3DVizualize` or `scripts/build_example_robot3dvisualize_msvc.bat`.

## UI And Styling Rules

- Design UI layout in `mainwindow.ui` using Qt Designer/Qt Creator. Do not hard-code the whole UI layout in C++ source.
- C++ may create runtime-only VTK objects and small dynamic bindings, but persistent panels, controls, labels, tables, actions, menus, and layout structure belong in `.ui`.
- Do not hard-code stylesheet strings in C++.
- If styling is needed, put it in a `.qss` file and include it through a `.qrc` resource registered from `Robot3DVizualize.pro`.
- Code may load/apply the stylesheet from the Qt resource path, for example `:/styles/robot3dvisualize.qss`, but the stylesheet content must live in the `.qss` file.
- Any new `.ui`, `.qrc`, `.qss`, asset, source, or header file must be listed in `Robot3DVizualize.pro` so Qt Creator builds exactly the same project the scripts build.

## Runtime Asset Policy

Use STL files for runtime rendering:

- `MZ04-01_base.stl`
- `MZ04-01_j1.stl`
- `MZ04-01_j2.stl`
- `MZ04-01_j3.stl`
- `MZ04-01_j4.stl`
- `MZ04-01_j5.stl`
- `MZ04-01_j6.stl`
- `Centering_tool.stl`

Keep STEP files as CAD source references. Do not attempt direct STEP loading in Phase 1.

## Implementation Order

1. Phase 1: load and render STL actors in a static scene.
2. Phase 2: map STL actors to canonical link poses.
3. Phase 3: FK controls and pose readout.
4. Phase 4: IK controls and solution table.
5. Phase 5: arm-config/posture controls.
6. Phase 6: final docs and manual QA.

## Phase 6 Expectations

For Phase 6, keep the slice focused:

- document the example build flow, including the core-library dependency;
- document the active tool/reference-frame behavior and posture controls;
- capture a concise manual QA checklist;
- note the current home-relative CAD alignment assumption explicitly.

Do not change solver behavior, visual alignment assumptions, or core library APIs as part of Phase 6 docs work.

## Coordinate And Unit Rules

- Core RobotKinematics math uses meters/radians.
- UI may display millimeters/degrees, but names and labels must make units explicit.
- Nachi teach-pendant pose order is `X, Y, Z, RZ, RY, RX`.
- `Pose::fromXYZRPY_*` expects `roll, pitch, yaw`, so map pendant pose as `rx, ry, rz`.
- The example-local STL-backed centering tool uses flange/TCP offset `(45, 0, 112, 0, 0, 0)`.
- Do not silently reuse the older exploratory example value `(44.2, 0, 139.0)` for `Centering_tool.stl`.

## Verification

For each handoff:

- Run `scripts\build_example_robot3dvisualize_msvc.bat`.
- Run `scripts\test_msvc.bat` to prove the core library remains independent from VTK.
- If UI/runtime changed, launch the example and confirm it starts without crashing.
- Note any visual alignment uncertainty explicitly in `HANDOFF.md`.

## Do Not Do

- Do not claim the STL visual alignment is calibrated to the real robot.
- Do not bypass `Robot3DVizualize.pro` when adding files or build settings.
- Do not hard-code UI layout or stylesheet contents in C++.
- Do not implement Kawasaki, SCARA, delta, or path planning here.
- Do not implement collision checking in this example until the core primitive collision API exists;
  after that, visualize core collision results only and do not use VTK/STL mesh intersections as
  collision truth.
- Do not put Nachi-specific visual data into the core solver.
- Do not silently convert units.
