# Phase 1 Baseline Verification

**Date:** 2026-06-24  
**Status:** Verified for the current MSVC/Qt Debug development baseline

## Environment

- Qt: `C:\Qt\6.8.2\msvc2022_64`
- Compiler: Visual Studio 2022 Community MSVC, initialized through
  `VC\Auxiliary\Build\vcvars64.bat`
- OpenCV: `C:\opencv\build`, linked as `opencv_world4120d` for Debug
- Basler Pylon: v11 import/runtime DLLs from `C:\Program Files\Basler\pylon`
- RobotKinematics third-party runtime: repo-root `3rdparty/{coal,assimp,boost}`

## Build Method

Use qmake from the build directory, then the make tool selected by the kit. For
Codex CLI verification, the reliable MSVC path is:

1. `call vcvars64.bat` without redirecting output.
2. Verify `VCToolsInstallDir` and `INCLUDE`.
3. Add Qt/OpenCV/Basler/runtime output directories to PATH.
4. Run `qmake -o Makefile <project>.pro -spec win32-msvc CONFIG+=debug`.
5. Run `nmake /nologo`.

Qt Creator can use `jom`, but Phase 1 found that `jom` launched from the Codex
shell did not inherit the PATH set inside the `cmd` wrapper and failed to find
`cl`. See [build_and_verification.md](build_and_verification.md) for the exact
commands and fallback guidance.

## Changes Made During Verification

- `components/RobotKinematics/robotkinematics.pri` now copies Coal's transitive
  Boost runtime DLLs:
  - `boost_serialization-vc143-mt-x64-1_87.dll`
  - `boost_filesystem-vc143-mt-x64-1_87.dll`
- `MatchedObject::setPossibleToPick(bool)` is public so architecture tests can
  construct a realistic matcher output DTO.
- `CameraWorkspace` is declared/registered as a Qt metatype.
- `TaskLocalization` and `LocalizationRuntimeController` register
  `CameraWorkspace` for queued matching payloads.
- `tests/architecture_contract_test` now includes
  `test_runtime_matching_payload_metatypes_support_queued_connection`.

## Verification Results

- `tests/architecture_contract_test` rebuilt successfully in
  `build/phase1_architecture_contract_20260623_debug`.
- `architecture_contract_test.exe -functions` includes
  `test_runtime_matching_payload_metatypes_support_queued_connection`.
- `architecture_contract_test.exe -silent` exited with code `0`.
- `ncr_picking.pro` rebuilt successfully in
  `build/phase1_ncr_picking_20260623_debug`.
- Final app artefact:
  `build/phase1_ncr_picking_20260623_debug/debug/ncr_picking.exe`.
- Post-link RobotKinematics deployment verified in the app binary directory:
  - `coal.dll`
  - `assimp-vc143-mt.dll`
  - `boost_serialization-vc143-mt-x64-1_87.dll`
  - `boost_filesystem-vc143-mt-x64-1_87.dll`
  - `robot_assets/Nachi/MZ04` with 52 files copied

## Warnings Observed

These warnings pre-exist the Phase 1 fixes and did not block the baseline:

- `C4100` unreferenced parameters in task/matching code.
- `C4244` narrowing conversions in matching geometry code.
- `C4996` deprecated `ads::CDockWidget` constructor use in `mainwindow.cpp`.
- Qt moc warning: `ItemPickingCenter` implements `QGraphicsItem` but does not
  list it in `Q_INTERFACES`.

## Remaining Risks

- Clean customer-install verification is still open: installer packaging must
  ship Qt/OpenCV/Basler dependencies, RobotKinematics DLLs, and `robot_assets/`.
  Track the release checklist in
  [customer_installer_packaging.md](customer_installer_packaging.md).
- The architecture contract test still needs the explicit
  `compiler_moc_source_make_all` step because it includes inline `main.moc`.
- Use Qt Creator's `jom` only from the configured IDE kit. From Codex CLI, keep
  the qmake+nmake fallback unless the `jom` PATH inheritance issue is resolved.
