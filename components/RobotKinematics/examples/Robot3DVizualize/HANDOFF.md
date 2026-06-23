# Robot3DVizualize Handoff

## Status

Phase 6 plus primitive collision visualization follow-up work are complete.

Verified:

- `scripts\build_example_robot3dvisualize_msvc.bat` builds the example with VTK `9.6`.
- `scripts\test_msvc.bat` passes, so the core build/test path remains independent from VTK.
- The example starts without an immediate runtime crash when Qt and VTK DLL paths are on `PATH`.
- The example still starts without an immediate runtime crash after adding the collision tab and scene highlighting.
- The VTK scene renders the eight STL runtime assets and updates actor transforms from FK as joints change.
- FK joint controls, readouts, IK solving, posture controls, and apply-selected-solution flow are wired through the live UI.
- The Debug tab can isolate per-mesh visibility, per-mesh origins, and per-joint FK axes for CAD/export debugging.
- The new Collision tab loads the primitive Nachi collision profile, supports an optional safety margin in millimeters, reports current pair diagnostics, and highlights colliding links in the VTK scene using core pair-level results.
- This handoff verified build + startup smoke for the new collision UI. A desktop operator should still do one in-app safe-state pass and one colliding-state pass before treating the example QA as fully closed.
- Missing or unreadable STL assets surface a visible warning dialog plus a status-bar message.
- The README now documents build/run/runtime setup plus a manual QA checklist.

Current local MSVC script default for `VTK_ROOT`:

```bat
D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs
```

## What Landed In Phase 2-5

- The example now links to the core `RobotKinematics` static library through `Robot3DVizualize.pro`, and the example build script builds the core library first.
- Added `Robot3DVisualizerLogic.h` for testable example-local logic such as Nachi pendant pose conversion, posture label mapping, and home-relative visual transform conversion.
- Added `Robot3DVisualizerLogicTests` to the repository Qt Test suite.
- Redesigned `mainwindow.ui` into a persistent split layout with FK, IK, and posture tabs plus a dedicated VTK host panel.
- Added QSS loading from the Qt resource system.
- Mapped each runtime STL mesh to a canonical Nachi link id and now update actors from `ForwardKinematics::computeChain`.
- Added FK joint control UI, flange/TCP readout, sample fixture buttons, and active tool/reference selectors.
- Added IK target entry, `solve`/`solveAll`, solution table, and apply-selected-solution behavior.
- Added current posture readout plus posture preference/requirement controls for IK.
- Corrected the example-local `Centering_tool.stl` handling: the visualizer now uses the STL-backed tool offset `(45, 0, 112, 0, 0, 0)` and a static CAD-frame correction before the flange FK delta is applied.

## Visual Alignment Assumption

- The current visual mapping assumes each STL mesh is authored in assembled CAD-world coordinates for the robot's home pose.
- Actor motion is computed as `currentLinkPose * inverse(homeLinkPose)` and only translation is scaled from meters to millimeters for rendering.
- `Centering_tool.stl` is a documented exception: the raw asset was exported in a different CAD frame, so the example applies a static home correction that maps the STL mount marker near `(-352, 625, 0)` and STL tip marker near `(-464, 625, 45)` onto the validated flange/TCP offset direction.
- This assumption is explicit and example-local. It does not modify solver logic or the Nachi preset itself.

## VTK Modules Added Beyond Phase 1

`vtk_config.pri` now also links:

- `vtkCommonMath` for `vtkMatrix4x4`

## Phase 6 Outcome

- The example is now documented end-to-end for another developer: VTK setup, build, runtime DLL path handling, UI behavior, and manual QA.
- The current visual alignment assumptions remain explicit, including the fact that CAD export origins may require example-local correction.
- The new Debug tab exists specifically so future CAD-origin issues can be localized without guessing whether the problem is mesh origin, joint axis placement, or tool/TCP configuration.

## Known Unknowns

- The STL local coordinate frames are still not calibrated against vendor CAD or a measured robot. The current home-relative mapping is visually plausible but not a calibration claim.
- Mesh units are still assumed to be millimeters. The current renderer scales only FK translation into that visual world.
- The example folder name has a typo: `Robot3DVizualize`. Do not rename it mid-phase unless the user approves, because Qt Creator project state may depend on the existing path.
- The rendered `Centering_tool.stl` mesh is always the visual CAD asset on the flange. Active TCP selection changes FK/IK math and readouts, but does not swap the visible mesh.
- The core Nachi preset reference doc still contains the older measured fixture offset `(44.2, 0, 139.0) mm` for non-example FK checks. Do not reuse that value for `Centering_tool.stl`; this example uses `(45, 0, 112) mm`.
- The example still relies on example-local CAD alignment corrections rather than a formal per-link calibration dataset.
- Collision visualization is not part of the current example implementation. Add it only after the
  core primitive collision API exists, and use primitive collision results rather than VTK/STL mesh
  intersections as collision truth.
  This is now implemented for the Nachi example using conservative primitive profiles.

## Verification Commands

- `scripts\build_example_robot3dvisualize_msvc.bat`
- `scripts\test_msvc.bat`

For runtime smoke verification, ensure Qt and VTK DLL directories are on `PATH` before launching the built executable.
