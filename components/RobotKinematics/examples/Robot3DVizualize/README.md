# Robot3DVizualize

Qt 6/qmake + VTK example for visualizing the Nachi MZ04D robot.

This example intentionally keeps VTK outside the core RobotKinematics library. The default repository build does not depend on VTK.

## Current Status

Phase 6 is complete:

- qmake VTK dependency detection is configured.
- the example links against the core `RobotKinematics` static library and the example build script builds that library first.
- the app renders the eight STL runtime assets and updates them from live FK using explicit home-relative visual transforms.
- `Centering_tool.stl` now uses the validated example-local flange/TCP offset `(45, 0, 112, 0, 0, 0)` plus a static CAD-frame correction before the FK delta is applied.
- orientation axes, a simple wireframe ground plane, lighting, and per-part materials are enabled.
- persistent FK, IK, and posture controls now live in `mainwindow.ui`.
- persistent collision status, primitive-profile source info, safety-margin control, and pair diagnostics now live in `mainwindow.ui`.
- a Debug tab now lets developers show/hide each mesh, each mesh origin, and each FK joint axis for visual alignment work.
- links involved in primitive self-collision are highlighted directly in the VTK scene using core pair-level collision results.
- flange/TCP readouts, `solve`, `solveAll`, posture preference/requirement controls, and apply-selected-solution flow are implemented.
- missing or unreadable STL assets are reported to the user through the status bar and a warning dialog.
- a startup smoke check passes when Qt and VTK DLL paths are on `PATH`.
- build/run/manual-QA documentation is now captured in this README and the example handoff docs.

See [IMPLEMENTATION_PLAN.md](IMPLEMENTATION_PLAN.md).
For agent handoff, read [AGENTS.md](AGENTS.md) and [HANDOFF.md](HANDOFF.md).

## Required External Dependencies

- Qt 6 for MSVC.
- MSVC toolchain.
- VTK built for the same compiler and Qt version family.

Set these environment variables before building:

```bat
set VTK_ROOT=D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs
set VTK_VERSION=9.6
```

The MSVC build script uses `D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs` automatically when `VTK_ROOT` is not set and that directory exists.

Optional overrides:

```bat
set VTK_INCLUDEPATH=D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs\include\vtk-9.6
set VTK_LIBPATH=C:\path\to\vtk-install\lib
set VTK_BINPATH=C:\path\to\vtk-install\bin
```

## Build

From the repository root:

```bat
scripts\build_example_robot3dvisualize_msvc.bat
```

The example build script now builds the core `RobotKinematics` library first, because the visualizer links against the repository static library instead of duplicating solver code.

If `VTK_ROOT` is not set and the default local VTK path does not exist, the script fails before qmake with a clear message. This is expected on machines without VTK installed.

VTK must be built with Qt support so `QVTKOpenGLNativeWidget.h` and `vtkGUISupportQt-<version>.lib` are available.

The current local VTK install also includes `vtkSTLReader.h` and `vtkIOGeometry-9.6.lib`, so Phase 1 should use STL assets directly.

## Run

After building, make sure Qt and VTK runtime DLL directories are on `PATH` before launching:

```bat
set PATH=C:\Qt\6.8.2\msvc2022_64\bin;D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs\bin;%PATH%
examples\Robot3DVizualize\build\msvc\release\Robot3DVizualize.exe
```

If the executable starts but the scene is blank or the process exits early, verify:

- Qt and VTK were built for the same MSVC toolchain.
- `vtkGUISupportQt-<version>.dll` and the Qt OpenGL/Widgets DLLs are on `PATH`.
- the executable can still locate `presets/Nachi/MZ04` from either the build output tree or the repository tree.

## Qt Creator Project Rules

Open and build the example from `Robot3DVizualize.pro`. This `.pro` file is the canonical project file for Qt Creator and the build script.

- Add new source/header/form/resource files to `Robot3DVizualize.pro`.
- Design persistent UI controls in `mainwindow.ui`.
- Do not hard-code the full UI layout in C++.
- Put stylesheet contents in `.qss` files.
- Include QSS through a `.qrc` resource declared by `Robot3DVizualize.pro`; C++ should only load the resource path.

## Visual Assets

Runtime mesh assets live in `presets/Nachi/MZ04/`:

- `MZ04-01_base.stl`
- `MZ04-01_j1.stl`
- `MZ04-01_j2.stl`
- `MZ04-01_j3.stl`
- `MZ04-01_j4.stl`
- `MZ04-01_j5.stl`
- `MZ04-01_j6.stl`
- `Centering_tool.stl`

The matching `.step` files are source CAD references and should not be used at runtime unless VTK CAD reader support is added later.

## Coordinate Reminder

Nachi teach-pendant pose data is written as `X, Y, Z, RZ, RY, RX`. RobotKinematics `Pose::fromXYZRPY_*` expects `roll, pitch, yaw`, so the example must map pendant orientation as:

```cpp
Pose::fromXYZRPY_mm_deg(x, y, z, rx, ry, rz);
```

## UI Behavior

Kinematics context:

- `Active tool` affects FK TCP readout and IK solve/target interpretation.
- `Reference frame` affects FK readout and IK target interpretation together; `Base` means raw base-frame values.
- The rendered tool mesh stays attached to the flange as a visual CAD asset. Changing the active tool changes math/readouts, not the visible mesh selection.

FK / IK / Posture / Debug tabs:

- `FK` drives live joint motion and shows flange/TCP pendant-format readouts.
- `IK` supports `Copy Current TCP`, `Solve Best`, `Solve All`, and apply-selected-solution.
- `Posture` lets IK prefer or require Nachi shoulder/elbow/wrist branches.
- `Collision` shows the active primitive profile source, applies an optional safety margin in millimeters, reports the closest/current colliding pairs, and highlights colliding links in the scene.
- `Debug` is for CAD/frame inspection: per-object mesh visibility, per-object mesh origin markers, and per-joint FK axis display.

## Current Visual Assumptions

- STL meshes are currently treated as CAD-world millimeter geometry authored in the robot's assembled home pose.
- Scene motion uses the home-relative transform `currentLinkPose * inverse(homeLinkPose)` and scales only translation from meters to millimeters for rendering.
- `Centering_tool.stl` is the exception: the raw asset was exported in a different CAD frame, so the example applies a static home correction before the flange FK delta. The validated tool offset for this STL-backed example is `(45, 0, 112, 0, 0, 0)`, not the earlier prototype value `(44.2, 0, 139.0, 0, 0, 0)`.
- The rendered `Centering_tool.stl` mesh always follows the flange as a visual CAD asset. Active TCP selection affects FK/IK math and readout, but does not swap the visible mesh yet.

The most important placement-tuning hooks in code are:

- `mainwindow.cpp` `visualHomeCorrectionForPartKey()`: per-mesh CAD/local-frame correction.
- `mainwindow.cpp` `buildExampleConfig()`: example-local tool/TCP offset.
- `mainwindow.cpp` `updateSceneFromChain()`: final place where the VTK matrix is applied to each actor.

## Manual QA Checklist

Before handing off the example on a new machine or after mesh/correction changes, check:

1. Build the core library and the example with `scripts\build_example_robot3dvisualize_msvc.bat`.
2. Confirm the executable launches when Qt and VTK DLL paths are on `PATH`.
3. Verify all eight STL assets appear, and missing assets surface a visible warning plus status-bar message.
4. In `FK`, move one joint at a time and confirm only downstream links move.
5. In `FK`, compare `Home`, `Midpoint`, `Teach P1`, and `Teach P20` behavior against expected visual posture and readout changes.
6. In `IK`, use `Copy Current TCP`, then run `Solve Best`; confirm one valid solution can be applied back to the FK controls.
7. In `IK`, run `Solve All` twice for the same target and confirm result ordering/content is stable enough for local use.
8. In `Posture`, request a branch preference, then repeat with `Require selected posture branches to match` enabled and confirm the result set/status changes appropriately.
9. In `Debug`, show one mesh origin and one joint axis at a time to confirm STL export origins match the intended joint/frame assumptions.
10. Re-check any edited CAD alignment values after STL export changes; a corrected mesh origin may remove the need for a compensating `visualHomeCorrectionForPartKey()` offset.

## Non-Goals Reminder

- This example is not a calibration claim for the real robot.
- VTK remains example-only and must not leak into the core library build.
- STEP files are preserved as source CAD references, not runtime assets.
- Collision visualization now consumes the core primitive collision API and highlights colliding
  links from primitive pair results only. It does not use VTK/STL mesh intersections as collision
  truth.
