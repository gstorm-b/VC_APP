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

## Runtime Threading Model Decision (2026-06-24)

Decision: `LocalizationRuntimeController` runs on the `TaskRunner` coordinator
thread (`TaskRunner::m_runtimeThread`), devices keep their own per-device
threads, and template matching is offloaded to the dedicated `matchingRunner`
worker thread. This is the supported runtime design — the open
"decide whether the controller must move into `m_runtimeThread`" question is
resolved in the affirmative, and no further threading change is planned.

Rationale:

- The controller owns the cycle state machine (`m_cycleState`), recovery
  bookkeeping (`m_recoveryContexts`), active camera/pattern bindings, and PLC
  signal dispatch. All of these must be serialized against asynchronous PLC
  value events and active-camera/pattern changes. Pinning the controller to one
  coordinator thread provides that serialization for free: every entry point
  (`setup`, `execute`, `handlePlcValues`, `setActiveCameraNumber`,
  `setActivePatternGroupNumber`, `configure`) is marshalled onto that thread
  (`setup` via `BlockingQueuedConnection`, the rest via `QueuedConnection` from
  `TaskLocalization::queue*`), so there are no locks and no data races on the
  controller's mutable state.
- Devices keep their own `HighPriority` threads (camera grab, PLC polling,
  vision TCP I/O). The controller never blocks on device I/O; it coordinates
  through queued runner signals and the `DeviceCommand` queue.
- The expensive work — template matching — is already off the controller call
  stack. `runtimeMatchingRequested` is delivered to `m_matchingWorker` on the
  `matchingRunner` thread, and the result is posted back via a queued
  `onRuntimeMatchingFinished`. A long match therefore cannot stall the PLC
  handshake or the GUI.
- `TaskRunner::enterRuntime(mergeToTaskThread = true)` (collapse all devices
  onto the coordinator thread) is explicitly rejected for Localization:
  `beginRuntime` logs a warning and ignores the flag.

Revisit criteria: re-open only if Product Verification measures cycle latency or
PLC-handshake jitter that traces to the coordinator thread being a bottleneck.
Until then, "measure first" — no speculative re-threading.

## Still Open

- Operator UI verification against a real or simulated runtime cycle: dashboard
  lamps, fault panel, KPIs, result table, task-local log, and read-only
  dashboard behavior.
- Runtime matching latency/UI responsiveness measurement.
- ~~Fully runner-based runtime active-camera switching.~~ Resolved
  (2026-06-24). Reviewed the full switch path: `TaskLocalization::setCameraNumber`
  → `queueSetActiveCameraNumber` (queued onto the controller thread) →
  `LocalizationRuntimeController::setActiveCameraNumber`, which is fully
  runner-coordinated — it guards mid-cycle changes, rebinds the active camera
  role, disconnects the previous camera via `previousRunner->requestDisconnect()`
  and connects the new one via `newRunner->requestConnect()` (both through the
  per-device command queue), and per-cycle grab/command connections always target
  the current `activeCameraRunner()`. Fixed one latent defect in the process: the
  active-workspace resolution dereferenced the new runner unguarded after an
  early-return path, inconsistent with the `requestConnect` guard above it; the
  deref is now inside the `if (newRunner)` block and the workspace is refreshed
  before the runtime is marked ready. Verified by the architecture contract suite
  (38 passed).
- Customer installer clean-machine verification, tracked separately in
  [customer_installer_packaging.md](../../product/customer_installer_packaging.md).

