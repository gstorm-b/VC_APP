# Localization Task Implementation Plan

## Summary

Complete `TaskLocalization` according to the behavior contract in
`docs/task_localization.md`: rename the active pattern signal, add PLC-visible
fault reporting, implement the runtime trigger cycle, publish PLC outputs
through a generic PLC write API, send world-coordinate results to the
VisionOutput device, and promote the dashboard into a read-only operator
runtime view.

Implementation should be staged so each step can be reviewed and tested
independently:

1. Config/schema and signal naming.
2. Generic PLC output writing.
3. Runtime cycle and recovery logic.
4. Dashboard and task UI updates.
5. Tests and documentation cleanup.

Finalized architecture decisions:

- Keep per-device threads from Commission into Runtime. The task runtime thread
  is a coordinator/state-machine thread only; device I/O remains on each
  device runner thread.
- Dashboard v1 is a read-only operator runtime view and includes a task-local
  recent log monitor.

Current implementation status:

- Per-device threads are kept from Commission into Runtime.
- Device commands and PLC writes are routed through queued runner APIs.
- `LocalizationRuntimeController` is moved into `TaskRunner::runtimeThread()`
  during runtime and is not parented to `TaskLocalization` while moved.
- Task-to-controller runtime calls are queued invocations.
- Runtime matching runs asynchronously on the existing localization matching
  worker thread. The controller emits `runtimeMatchingRequested(...)` and
  accepts `onRuntimeMatchingFinished(cycleId, result)`.
- Active camera switching is runner-based; `TaskLocalization::setCameraNumber()`
  validates the request and queues it to the controller instead of directly
  connecting or disconnecting camera devices.

## Config, Signals, And Fault Codes

- Rename `nActivePattern` to `nActivePatternGroup`.
- Add `bTaskFault`.
- Add `nFaultCode`.
- Update `TaskLocalizeConfig`, JSON persistence, meta-property tables,
  `LocalizationSignalMapper`, setting widget signal rows, and dashboard signal
  rows.
- Do not add a compatibility shim for `nActivePattern`; the project is not
  deployed yet.

Stable fault-code values:

| Code | Name | Meaning |
| ---: | --- | --- |
| 0 | `None` | No active fault. |
| 100 | `CameraLost` | The active camera lost connection. |
| 101 | `CameraConnectFailed` | The active camera could not connect. |
| 102 | `CameraGrabTimeout` | The single-shot grab timed out. |
| 200 | `VisionOutputLost` | The vision-output device lost connection. |
| 201 | `VisionOutputSendFailed` | The result payload could not be sent. |
| 300 | `PlcLost` | The primary PLC lost connection. |
| 400 | `PatternInvalid` | The active pattern group is missing or not usable. |
| 401 | `CalibrationInvalid` | The active camera has no valid calibration for world-coordinate output. |
| 500 | `InternalError` | Unexpected internal task error. |

## Generic PLC Write Path

- Add a PLC-family write capability, for example `IPlcIoWriter`, with:
  - `writeDigitalIoByName(const QString &tag, bool value)`.
  - `writeWordIoByName(const QString &tag, qint16 value)`.
- Implement the capability in `McProtocolDevice` by parsing existing tag names:
  - `M0000` style tags become bit write `MCRequest`s.
  - `D0000` style tags become word write `MCRequest`s.
  - Invalid or out-of-family tags are rejected and logged.
- Add queued write helpers to `PlcRunner` so task/runtime code does not call
  PLC device methods directly across threads.
- Add controller-level publishing helpers:
  - `publishBoolSignal(name, value)`.
  - `publishNumberSignal(name, value)`.
- Publishing a signal must both emit the task-facing live update and write the
  mapped PLC tag when the PLC is writable.

## Runtime Logic

- Fix runtime startup ordering so task runners are synchronized and started
  before runtime setup validates runner availability.
- Replace raw-pointer `shared_ptr::reset(raw)` camera handling with shared
  ownership from `DeviceManager`.
- Add explicit matching worker teardown.
- Register queued metatypes required by matching/commission signals.
- Add an internal runtime cycle state in `LocalizationRuntimeController`:
  - `NotReady`
  - `ReadyForTrigger`
  - `Running`
  - `WaitingTriggerReset`
  - `Recovering`
  - `Faulted`
- Runtime startup validates:
  - Primary PLC binding and runner.
  - VisionOutput binding and runner.
  - At least one camera-number binding.
  - Active camera binding and runner.
  - Active pattern group exists and has train images.
  - Active camera calibrator is valid.
- Runtime startup connects only the required runtime devices:
  - Primary PLC.
  - VisionOutput device.
  - Active camera.
- Runtime uses the default per-device-thread mode. Do not merge camera, PLC, or
  VisionOutput devices into the task runtime thread for localization runtime.
- A localization cycle starts only on `bExecuteTrigger` rising edge while the
  internal state is `ReadyForTrigger`.
- During a cycle:
  - Publish busy/ready handshake outputs.
  - Request a single-shot frame through `CameraRunner`.
  - Snapshot the active pattern group before matching.
  - Run matching after the frame is received by emitting a worker-thread
    request from the runtime controller.
  - Convert matched image coordinates to world coordinates using the active
    camera calibrator.
  - Send a `VisionOutputRequest` through the assigned VisionOutput device.
  - Publish success outputs and wait for trigger reset.
- A repeated trigger while `Running` or `WaitingTriggerReset` must not enqueue a
  second cycle.

## Fault And Recovery Behavior

- `bMatchingFinished` means the accepted trigger has been handled. It does not
  mean the cycle succeeded.
- A successful cycle publishes `bTaskFault = false` and `nFaultCode = 0`.
- An abnormal cycle failure publishes `bTaskFault = true` and a non-zero
  `nFaultCode`.
- On cycle fault, publish when PLC is writable:
  - `bMatchingBusy = false`
  - `bMatchingFinished = true`
  - `bTaskReady = false`
  - `bMatchingDetected = false`
  - `bMatchingLowArea = false`
  - `nDetectedNumber = 0`
  - `bTaskFault = true`
  - `nFaultCode = <specific fault code>`
- Grab timeout is `CameraGrabTimeout`, not a valid zero-detection result.
- Do not send normal `0;` VisionOutput payload when no valid frame was captured.
- If PLC is disconnected, task state and application logs are the source of
  truth until reconnect. After PLC recovery, republish the current fault state
  instead of silently clearing it.
- Recovery defaults remain:
  - 10 retries.
  - 5000 ms interval.
  - Recoverable statuses: `LostConnected`, `ConnectFailed`.

## Dashboard And UI

- Setting widget:
  - Replace `nActivePattern` row with `nActivePatternGroup`.
  - Add `bTaskFault`.
  - Add `nFaultCode`.
- Dashboard v1 is a read-only operator runtime view.
- Remove or disable the current unused Start button.
- Left pane should show:
  - Assigned VisionOutput device.
  - Assigned PLC device.
  - Active camera.
  - Active pattern group.
  - Task state.
  - Fault state and fault code.
  - Signal monitor including `bTaskFault` and `nFaultCode`.
  - Task-local recent log monitor for the active localization task.
- Right pane should show:
  - Last runtime match image.
  - Runtime KPIs.
  - Last result table.
- Result table columns:
  - Index.
  - Pattern.
  - Score.
  - Image X.
  - Image Y.
  - Image R.
  - World X.
  - World Y.
  - World Z.
  - World R.
  - Sent/skipped reason.
- Add a task/runtime signal for dashboard updates carrying the last cycle
  result, converted output positions, fault code, and display image.
- Add a task/runtime signal or bounded task-local log buffer for dashboard log
  updates. The dashboard log is task-scoped and does not replace the global
  system log widget.
- Keep dashboard structure in `.ui`. Add themed QSS files only if visual
  styling changes are needed.

## Tests And Verification

- Config and mapper tests:
  - New signal keys serialize/deserialize.
  - `LocalizationSignalMapper` maps `nActivePatternGroup`, `bTaskFault`, and
    `nFaultCode`.
  - Fault-code numeric values are stable.
- PLC write tests:
  - Valid `M0000` tag writes digital output.
  - Valid `D0000` tag writes word output.
  - Invalid tags are rejected without crashing.
- Runtime/controller tests:
  - Rising trigger starts exactly one cycle.
  - Held trigger does not start another cycle.
  - Trigger reset clears `bMatchingFinished` and restores ready when healthy.
  - Grab timeout sets fault code `102` and does not send normal result.
  - VisionOutput send failure sets fault code `201`.
  - Invalid pattern group sets fault code `400`.
  - Invalid calibration sets fault code `401`.
  - Lost camera, PLC, or VisionOutput during `RunningCycle` aborts the cycle and
    follows recovery behavior.
- UI verification:
  - Setting widget shows all expected signal rows.
  - Dashboard monitor receives live values for new fault signals.
  - Dashboard result table and KPIs update after a runtime cycle.
  - Dashboard task-local log shows recent task events only.
  - Dashboard does not expose manual writes, reset, trigger, or runtime
    start/stop controls in v1.

## Assumptions

- Runtime dashboard is read-only in this implementation pass.
- Runtime start/stop remains outside the dashboard.
- PLC output writing is implemented through a generic PLC capability, not
  Mitsubishi-specific task code.
- `nActivePatternGroup` is a breaking rename from `nActivePattern`.
- Runtime controller remains the coordinator; device I/O stays on device
  runner threads, and runtime matching runs on the task matching worker thread.
- The dashboard task log is an operator-facing recent-event view, not a
  persistent audit log.

## Architecture Debt And Test Completion Plan

### Implementation Status

Implemented in the architecture-debt pass:

- Runtime controller ownership was moved to `TaskRunner::runtimeThread()` for
  runtime operation, with explicit create/destroy in `TaskLocalization`.
- `RuntimeContext` now snapshots config, runners, pattern groups, and camera
  calibrators before controller setup.
- Runtime matching now runs through the task matching worker path and returns by
  cycle id, so stale results are ignored.
- Active camera switching is handled by `LocalizationRuntimeController` through
  `CameraRunner`.
- Stable fault-code values were corrected to the agreed PLC contract
  (`100/101/102`, `200/201`, `300`, `400/401`, `500`).
- Focused fake-device runtime tests were added to
  `tests/architecture_contract_test` to cover trigger/held-trigger/reset,
  grab timeout, vision-output send failure, and camera-change rejection while
  running. They are colocated with the architecture contract suite instead of a
  new test target to avoid duplicating the large qmake source list.
- UML diagrams were updated in `uml/03_runtime_threading.puml` and
  `uml/04_localization_task.puml`.

### Goal

Move `TaskLocalization` from first implementation pass to production-candidate
software architecture. After this section is implemented, localization runtime
must have a clear thread ownership model, async runtime matching, runner-based
camera switching, and fake-device tests for trigger, fault, recovery, and PLC
write behavior.

### Runtime Controller Thread Ownership

- Move `LocalizationRuntimeController` to `TaskRunner::runtimeThread()` when
  localization enters runtime.
- Do not parent the controller to `TaskLocalization` while it is moved to the
  runtime thread. Own it with an explicit pointer and stop/delete it in
  `TaskLocalization` teardown after the runtime thread has stopped.
- Convert all task-to-controller calls that can happen during runtime into
  queued invocations:
  - `configure(config)`
  - `setupRuntime(context)`
  - `execute()`
  - `setActiveCameraNumber(number)`
  - `setActivePatternGroupNumber(number)`
  - `handlePlcValues(values)`
- Keep controller-to-task notifications as queued Qt signals:
  - `signalChanged`
  - `cycleResultUpdated`
  - `taskLogAppended`
  - `runtimeCycleStarted`
  - `runtimeRecovering`
  - `runtimeReady`
  - `runtimeFault`
- The controller must not call `TaskLocalization` methods directly after it is
  moved into `TaskRuntimeThread`.

### Runtime Context Snapshot

- Add a runtime context/snapshot type that is built on the task object thread
  before the controller setup is invoked.
- The context must contain all runtime data the controller needs without
  calling back into `TaskLocalization`:
  - copied `TaskLocalizeConfig`
  - primary PLC device id and runner pointer
  - vision-output device id and runner pointer
  - camera-number to camera-runner map
  - active camera number
  - active pattern group number
  - immutable pattern group snapshot for each runtime-usable group
  - calibration snapshot for each runtime-usable camera
- Pattern and calibration edits are not supported while runtime is active. To
  refresh them, stop runtime, edit in commission/settings, then start runtime
  again.

### Async Runtime Matching

- Runtime matching must run on the existing `TaskLocalization::matchingRunner`
  worker thread, not synchronously in the runtime controller.
- Add a worker QObject that receives:
  - cycle id
  - pattern group snapshot
  - frame clone
- Worker runs `LocalizationPipeline::runMatch(...)` and emits:
  - cycle id
  - `mtc::MatchResult`
  - elapsed matching time
  - display image
- Controller must ignore stale matching results whose cycle id is no longer the
  active cycle.
- Aborting a cycle must leave any late matching result harmless.

### Runner-Based Active Camera Switching

- Remove direct runtime calls from `TaskLocalization::setCameraNumber()` to:
  - `CameraDevice::deviceDisconnect()`
  - `CameraDevice::deviceConnect()`
- `TaskLocalization::setCameraNumber()` should only validate the requested
  logical number enough to reject obvious invalid input, then queue the request
  to `LocalizationRuntimeController`.
- The controller owns runtime camera switching:
  - reject and log camera changes while `Running`
  - when ready/waiting-reset, publish `bTaskReady=false`
  - disconnect the old active camera runner if needed
  - bind the new camera runner as the active camera role
  - request connect through `CameraRunner`
  - validate calibration snapshot
  - publish `bCameraValid`
  - return to ready only when all required roles are healthy
- Camera switching must use the same recovery/fault policy as runtime startup.

### Runtime And PLC Tests

- Add a focused runtime test target, for example
  `tests/localization_runtime_controller_test`.
- Use fake devices/runners rather than hardware:
  - fake PLC captures `IPlcIoWriter` writes
  - fake camera emits configurable grab success, grab timeout/failure, and lost
    connection
  - fake vision output captures sent positions and can simulate send failure or
    lost connection
- Runtime/controller tests must cover:
  - rising trigger starts exactly one cycle
  - held trigger does not enqueue another cycle
  - trigger reset clears `bMatchingFinished` and restores `bTaskReady`
  - grab timeout sets fault code `102` and sends no normal vision output
  - vision-output send failure sets fault code `201`
  - invalid pattern group sets fault code `400`
  - invalid calibration sets fault code `401`
  - lost camera, PLC, or vision output during `RunningCycle` aborts the cycle
    and publishes the correct fault code
  - active camera change while `Running` is rejected
  - active camera change while ready is runner-based and restores ready when
    the new camera is healthy
- PLC write tests must cover:
  - valid `Mxxxx` digital tag write
  - valid `Dxxxx` word tag write
  - invalid tag rejection without crash
  - `signalChanged` is emitted even when the mapped PLC tag is empty

### Required Verification

- Run existing `architecture_contract_test`.
- Run the new localization runtime test target.
- Build the full Debug app.
- Search the runtime path to confirm no direct `CameraDevice::deviceConnect()`
  or `CameraDevice::deviceDisconnect()` calls remain in `TaskLocalization`.
- Update `uml/03_runtime_threading.puml` and `uml/04_localization_task.puml`
  after the code reflects the new controller ownership and async matching
  flow.
