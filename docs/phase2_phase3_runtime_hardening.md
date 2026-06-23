# Phase 2/3 Runtime Hardening Progress

**Date:** 2026-06-24  
**Status:** Partial Phase 2 hardening verified; low-risk Phase 3 cleanup verified

## Phase 2 Work Completed

Added focused architecture contract coverage for Localization runtime paths that
were listed in the roadmap but not yet locked by tests:

- Invalid active pattern group faults setup with `PatternInvalid` / fault code
  `400`.
- Invalid active camera calibration faults setup with `CalibrationInvalid` /
  fault code `401`.
- Runtime ready output writes the configured valid PLC `M` bit tags and `D`
  word tags.
- Successful runtime cycle writes busy/finished/detected/fault/count PLC
  outputs through `PlcRunner`.
- `PlcRunner` emits errors when the PLC writer rejects invalid tag families
  such as a `D` tag for digital output or an `M` tag for word output.
- Active camera loss during `RunningCycle` aborts the cycle with `CameraLost` /
  fault code `100` before any VisionOutput request is sent.

The fake PLC in `tests/architecture_contract_test` now mirrors the production
Mitsubishi MC tag-family rule: digital outputs must be `M<number>` and word
outputs must be `D<number>`.

## Phase 3 Cleanup Completed

- `IDeviceWidget` now has an explicit virtual destructor and
  `Q_DISABLE_COPY_MOVE`.
- `BaslerCameraWidget` now uses `qobject_cast<BaslerGigECamera*>` at the widget
  boundary. A wrong subtype logs an error, disables the widget, and does not
  wire runner signals into Basler-only slots.
- `VisionTcpipDeviceWidget` now uses `qobject_cast<VisionTcpipDevice*>` at the
  widget boundary. A wrong subtype logs an error and disables the widget.
- `VisionTcpipDeviceWidget::m_output_device` and
  `BaslerCameraWidget::m_camera` are null-initialized; `saveConfig()` now
  guards the null output-device path.

## Verification

- Rebuilt `tests/architecture_contract_test` with qmake + `nmake /nologo`.
- Ran `architecture_contract_test.exe -silent`; exit code `0`.
- Rebuilt the full Debug app target `ncr_picking.pro` with qmake +
  `nmake /nologo`.
- Final app build passed in
  `build/phase1_ncr_picking_20260623_debug/debug/ncr_picking.exe`.

Warnings observed are existing non-blocking warnings: unused parameters,
deprecated `ads::CDockWidget` constructor usage, and the existing Qt moc
interface warning.

## Still Open

- Operator UI verification against a real or simulated runtime cycle: dashboard
  lamps, fault panel, KPIs, result table, task-local log, and read-only
  dashboard behavior.
- Runtime matching latency/UI responsiveness measurement.
- Fully runner-based runtime active-camera switching. The controller guards
  camera changes while running, but the broader `TaskLocalization` camera switch
  path still needs review/refactor before declaring the item closed.
- Customer installer clean-machine verification, tracked separately in
  [customer_installer_packaging.md](customer_installer_packaging.md).

