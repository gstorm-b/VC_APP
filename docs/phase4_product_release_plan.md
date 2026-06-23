# Phase 4 Product Release Plan

**Date:** 2026-06-24  
**Status:** Product/packaging plan defined; customer installer implementation pending

## Release Shape Decision

The first commercial release should ship as **one Qt application with explicit
Commission and Runtime modes**.

Do not split into separate configuration and runtime executables for the first
release. The current codebase, runtime controller, task runner lifecycle, and
architecture contract tests are already organized around one application with
phase transitions. A two-executable split is a later product decision after the
single-app operator flow is validated.

## Why Single App First

- The current `TaskLocalization` lifecycle already models Commission and
  Runtime explicitly.
- The Phase 1/2 verification baseline is built around `ncr_picking.pro` and the
  architecture contract test suite.
- Splitting now would add project-file compatibility, IPC/shared-service
  boundaries, installer payload variants, and operator permission rules before
  the Runtime UI has completed a real or simulated operator pass.
- The project has not shipped to customers yet, so the first release should
  reduce moving parts and validate the critical Localization runtime path.

## Product Layout For First Release

The installed package should contain:

- `ncr_picking.exe`.
- Qt runtime DLLs and required plugin directories.
- OpenCV runtime DLLs matching the MSVC/Qt kit.
- Basler Pylon runtime dependency, either bundled if licensing permits or listed
  as a prerequisite installer.
- Advanced Docking System runtime DLLs if the final build links it dynamically.
- RobotKinematics transitive runtime DLLs and mesh assets, as detailed in
  [customer_installer_packaging.md](customer_installer_packaging.md).
- License notices for shipped third-party components.
- A release smoke-test checklist and known-risk notes.

## Runtime Operator Flow

The first-release operator path should remain:

1. Open the single app.
2. Load or create a project.
3. Enter Commission mode to configure devices, camera mappings, calibration,
   pattern groups, VisionOutput, and RobotKinematics advisory checking.
4. Start Runtime mode.
5. Confirm task ready/fault outputs and dashboard state.
6. Let PLC trigger Localization cycles.
7. Recover or stop through the existing task/runtime lifecycle.

Runtime screens must stay operator-safe:

- Dashboard is read-only.
- No manual PLC writes, trigger buttons, or destructive configuration controls
  are exposed in the Runtime dashboard.
- Fault state must show the stable fault code and readable reason.
- Device reconnect/recovery behavior must be visible enough for support.

## Packaging Work Items

Use [customer_installer_packaging.md](customer_installer_packaging.md) as the
payload checklist. The next implementation slice should:

- Choose installer technology: Qt Installer Framework, WiX, Inno Setup, or a
  company-standard packager.
- Create a packaging manifest that is separate from `QMAKE_POST_LINK`.
- Add a `windeployqt` collection step, then audit/copy non-Qt dependencies.
- Add RobotKinematics DLL and `robot_assets/` install rules.
- Add third-party license files to the installer payload.
- Create a clean-machine smoke-test record template.
- Decide whether Basler Pylon is bundled or documented as a prerequisite.

## Release Gate

Do not mark Phase 4 complete for shipment until:

- Full Debug or Release build succeeds from the documented qmake flow.
- Architecture contract test suite passes.
- The app launches on a clean machine without source-tree or Qt Creator PATH
  dependencies.
- The installed app can find `robot_assets/Nachi/MZ04` from the install
  directory.
- A simulated or real Localization runtime cycle verifies PLC outputs,
  VisionOutput send, dashboard lamps/faults, result table, and task log.
- Missing-DLL analysis is captured before changing PATH on the target machine.

## Deferred Product Split

A future two-executable split may still be valuable, but only after the
first-release runtime is validated. Revisit it when there is a concrete need for
one of these:

- locked-down operator station with no configuration access;
- separate deployment/update cadence for configuration tools and runtime;
- headless or service-style runtime;
- remote project authoring with local machine runtime.

Until then, keep shared project files, device modules, and runtime services in
the single app.

