# Nachi MZ04D VTK Visualizer Implementation Plan

## Goal

Build a Qt 6/qmake example app that visualizes the Nachi MZ04D preset in a real 3D scene using VTK.

This example is a consumer of the RobotKinematics API. It must not make VTK a dependency of the core library or the default repository build.

## Scope

- Example-only Qt Widgets app.
- Build system: Qt 6 qmake.
- Renderer: VTK embedded in Qt through `QVTKOpenGLNativeWidget`.
- Robot preset: `RobotKinematics::Presets::nachiMZ04D()` only.
- Kinematics features: FK, IK, and arm-config/posture classification.
- Visual assets: STL files in `presets/Nachi/MZ04/` are the runtime source for Phase 1. STEP files are preserved as CAD source references.

## Non-Goals

- No VTK vendoring in this repository.
- No change to the core `RobotKinematics.pro` build.
- No controller/network connection.
- No path planning, dynamics, or trajectory generation.
- No collision checking until the core primitive collision API from
  `docs/planning/collision_detection_plan.md` exists. After that lands, this example may visualize core
  collision results, but must not use VTK/STL mesh intersections as collision truth.
- No physical robot accuracy claim.
- No support for presets other than Nachi MZ04D in the first example.

## Dependency Policy

VTK is an external developer dependency.

The example build must be explicit about where VTK comes from:

- `VTK_ROOT`: VTK install prefix.
- `VTK_VERSION`: optional VTK include/lib suffix, for example `9.6`.
- `VTK_INCLUDEPATH`: optional override for VTK headers.
- `VTK_LIBPATH`: optional override for VTK libraries.
- `VTK_BINPATH`: optional runtime DLL path.

If VTK is missing, the example should fail at qmake/script time with a clear message. The core library and tests must still build without VTK.

## Qt Project, UI, And Styling Policy

- `Robot3DVizualize.pro` is the canonical Qt Creator project file for this example.
- Build/test scripts must use this `.pro`; agents must not create an alternate project entrypoint unless the user explicitly approves.
- Persistent UI layout must be designed in `.ui` files, starting with `mainwindow.ui`.
- Do not hard-code the full UI layout in C++ source files.
- Stylesheet contents must live in `.qss` files, not C++ string literals.
- QSS files must be included through a Qt resource file (`.qrc`) and that resource file must be listed in `Robot3DVizualize.pro`.
- Any new source, header, form, resource, stylesheet, or asset dependency must be registered through `Robot3DVizualize.pro` so Qt Creator and script builds stay identical.

## Phase 0: VTK Build Spike

**Objective:** Prove the example can detect and build against an external VTK installation without touching the core library build.

Tasks:

- [x] Keep the example isolated under `examples/Robot3DVizualize`.
- [x] Add qmake VTK configuration in `vtk_config.pri`.
- [x] Add a minimal Qt + VTK render-window smoke path.
- [x] Add an MSVC build script for the example.
- [x] Wire the local external VTK install path `D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs` as the script default when `VTK_ROOT` is unset.
- [x] Build the smoke app on a machine with VTK installed.
- [x] Confirm required VTK modules for Qt embedding.
- [x] Confirm whether the available VTK package can read STL directly.

Current Phase 0 status:

- The local VTK install is detected as VTK `9.6`.
- The local CUDA + Qt VTK install includes `QVTKOpenGLNativeWidget.h` and `vtkGUISupportQt-9.6.lib`.
- The smoke example builds and starts without an immediate runtime crash when Qt and VTK DLL paths are present.
- The current VTK install exposes `vtkSTLReader.h` and `vtkIOGeometry-9.6.lib`; Phase 1 should load STL files directly.
- The current VTK install does not expose a direct STEP/OCCT reader. Keep STEP files only as source CAD references unless the user explicitly asks to rebuild VTK with CAD reader support.

Acceptance criteria:

- Running the core build/test scripts does not require VTK.
- Running the example build without a valid `VTK_ROOT` and without the documented local script default fails with a clear actionable error.
- Running the example build with a valid VTK install opens a blank VTK render window.

## Phase 1: Static Scene And STL Loading

**Objective:** Show the Nachi STL parts in a VTK scene.

Tasks:

- [x] Load the seven robot STL files plus `Centering_tool.stl`.
- [x] Keep the matching STEP files untouched as CAD source references.
- [x] Add camera, axes, grid/floor, and simple lighting.
- [x] Add per-part colors/materials for readable link separation.
- [x] Report a clear error if any required STL file is missing.

Current Phase 1 status:

- The scene now loads all eight STL assets from `presets/Nachi/MZ04/` using `vtkSTLReader`.
- Asset lookup starts from `QCoreApplication::applicationDirPath()` and walks fallback repo-relative locations for local development.
- The scene adds a gradient background, a camera light, a wireframe ground plane, and an orientation marker widget.
- Missing or unreadable meshes show a visible warning dialog and status-bar message instead of failing silently.
- Phase 1 required extra VTK modules in `vtk_config.pri`: `vtkCommonColor` for `vtkNamedColors`, `vtkCommonDataModel` and `vtkCommonExecutionModel` for the STL/pipeline data flow, `vtkFiltersSources` for the ground plane, and `vtkInteractionWidgets` for `vtkOrientationMarkerWidget`.

Acceptance criteria:

- The app opens and renders the complete robot model.
- All expected STL parts are present.
- Missing/unreadable asset errors are visible in the UI/status bar.

## Phase 2: Link Transform Mapping

**Objective:** Bind CAD actors to canonical link poses.

Tasks:

- [x] Map each CAD part to its canonical link id.
- [x] Apply FK `linkPosesInBase` transforms to actors.
- [x] Add per-part static `cadToLink` alignment transforms where CAD local frames differ from canonical link frames.
- [x] Document any alignment values and their source.

Current Phase 2 status:

- Each STL actor is retained and mapped to a canonical Nachi link id in `mainwindow.cpp`.
- The scene uses `RobotKinematics::Presets::nachiMZ04D()` plus `ForwardKinematics::computeChain`.
- Visual motion assumes each CAD mesh is authored in assembled home-pose coordinates; actor transforms use `currentLinkPose * inverse(homeLinkPose)` and scale translation from m to mm for rendering.
- `Centering_tool.stl` is now treated as an explicit exception: the raw STL is corrected from its export CAD frame before the flange home-relative delta is applied, and the example-local tool offset is `(45, 0, 112, 0, 0, 0)`.
- This is an explicit example-local visual assumption, not a solver or preset-data change.

Acceptance criteria:

- Home pose visually matches the expected Nachi arrangement.
- Moving one joint only moves the downstream visual parts.
- FK readout remains in SI internally and displays mm/degree explicitly.

## Phase 3: FK UI

**Objective:** Let the user drive the visualizer from joint values.

Tasks:

- [x] Design the persistent FK controls in `mainwindow.ui`.
- [x] Add six joint controls in degrees.
- [x] Validate joint limits with `JointLimitValidator`.
- [x] Display flange/TCP pose as `X, Y, Z, RZ, RY, RX` for Nachi-style pendant comparison.
- [x] Add reset/home and sample fixture buttons.
- [x] If styling is needed, add/update a `.qss` file through the `.qrc` resource listed in `Robot3DVizualize.pro`.

Current Phase 3 status:

- `mainwindow.ui` now owns the persistent FK layout: joint controls, sample buttons, context selectors, and flange/TCP readouts.
- The example loads its stylesheet from the Qt resource system instead of hard-coding style strings in C++.
- Joint input is clamped to the configured Nachi joint limits and updates the scene/readout immediately.
- The example includes sample buttons for home, midpoint, and two documented teach-pendant joint fixtures.

Acceptance criteria:

- Joint edits update the scene immediately.
- Out-of-limit joints are rejected or clearly marked.
- FK readout matches existing Nachi fixture expectations.

## Phase 4: IK UI

**Objective:** Let the user solve target poses and apply solutions.

Tasks:

- [x] Design the persistent IK controls and result table in `mainwindow.ui`.
- [x] Add target input as `X, Y, Z, RZ, RY, RX`.
- [x] Convert Nachi pendant order to `Pose::fromXYZRPY_mm_deg(x, y, z, RX, RY, RZ)`.
- [x] Call `SerialRobotKinematics::solve` and `solveAll`.
- [x] Show result table with J1..J6, residuals, score, and posture.
- [x] Allow applying the selected solution.

Current Phase 4 status:

- The IK tab now supports target pose entry in Nachi pendant order, target copy-from-current, `solve`, and `solveAll`.
- Solutions are shown in a persistent table with joints, TCP residuals, posture labels, and total score.
- Selecting a result and applying it pushes the solution back into the FK joint controls and scene.
- The example links against the core static library and uses public `SerialRobotKinematics` APIs instead of duplicating IK logic.

Acceptance criteria:

- A pose generated from current FK solves back to a valid joint solution.
- Unreachable or invalid targets show `KinematicsStatus` and message.
- `solveAll` results are deterministic for the same request.

## Phase 5: Arm Config/Posture

**Objective:** Expose Nachi arm-config behavior.

Tasks:

- [x] Design persistent posture controls in `mainwindow.ui`.
- [x] Classify current joints as righty/lefty, above/below, flip/non-flip.
- [x] Add posture request controls.
- [x] Support `requirePosture`.
- [x] Display when requested posture cannot be satisfied.

Current Phase 5 status:

- The posture tab shows current shoulder/elbow/wrist branch labels using the configured Nachi posture metadata.
- IK requests can now express posture preference or hard requirement through persistent combo-box controls plus `requirePosture`.
- Posture request labels are translated back into generic `ArmPosture` branches through the preset's `PostureResolver`.
- Unsupported or unsatisfied posture requests surface through the same visible IK status channel used for other solver failures.

Acceptance criteria:

- Classification matches the Nachi rules documented in `docs/preset_references/nachi-mz04d.md`.
- Requiring posture filters IK solutions or reports a structured failure.

## Phase 6: Documentation And Manual QA

**Objective:** Make the example usable by another developer.

Tasks:

- [x] Document VTK setup.
- [x] Document build command and runtime DLL path needs.
- [x] Document coordinate conventions and non-goals.
- [x] Add a manual QA checklist and screenshots if available.

Current Phase 6 status:

- `README.md` now documents the external VTK setup, build flow, runtime `PATH` requirement, UI behavior, and current visual assumptions.
- The README now includes a concise manual QA checklist for FK, IK, posture, and visual-debug checks.
- `AGENTS.md` and `HANDOFF.md` have been updated so future implementers inherit the correct Phase 6 context instead of re-opening already-complete work.
- The example now exposes enough in-app debug state to inspect mesh visibility, mesh origins, and FK joint axes while validating CAD exports.

Acceptance criteria:

- A developer with Qt 6, MSVC, and VTK can build/run the example from docs.
- The repo's default MSVC build/test remains independent from VTK.

## Open Decisions

- Exact VTK version and distribution to recommend.
- Whether to keep both STEP and STL in source control after the STL pipeline is proven.
- Whether to rename `Robot3DVizualize` to `NachiMZ04DVisualizer` after the initial spike.
