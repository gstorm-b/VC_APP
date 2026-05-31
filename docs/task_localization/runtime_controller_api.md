# LocalizationRuntimeController API

`LocalizationRuntimeController` is the runtime state machine and coordinator
for localization.

Source:

- `src/model/localization_runtime_controller.h`
- `src/model/localization_runtime_controller.cpp`

## Responsibility

The controller owns runtime behavior:

- PLC signal mapping and trigger handling.
- Runtime readiness and cycle state.
- Camera, PLC, and VisionOutput role bindings.
- Device connect/reconnect requests through runners.
- PLC output publishing through `PlcRunner`.
- Camera single-shot requests through `CameraRunner`.
- Vision result sends through `VisionOutputRunner`.
- Fault publishing and recovery state.
- Dashboard cycle result and task-local log events.
- Runtime matching request/response coordination by cycle id.

The controller does not own device objects, task objects, pattern managers, or
the matching pipeline.

## Thread Ownership

During runtime, `TaskLocalization` moves the controller to
`TaskRunner::runtimeThread()`. The controller must not be parented to
`TaskLocalization` while moved.

All public runtime calls from the task should be invoked with queued
connections when the controller is on another thread.

## RuntimeContext

```cpp
struct RuntimeContext {
    TaskLocalizeConfig config;
    QString primaryPlcDeviceId;
    QString visionOutputDeviceId;
    QPointer<PlcRunner> primaryPlcRunner;
    QPointer<VisionOutputRunner> visionOutputRunner;
    QMap<int, QString> cameraDeviceIds;
    QMap<int, QPointer<CameraRunner>> cameraRunners;
    QMap<int, std::shared_ptr<mtc::MatchGroup>> patternGroups;
    QMap<int, calib::Calibrator> cameraCalibrators;
    int activeCameraNumber{-1};
    int activePatternGroupNumber{-1};
};
```

`RuntimeContext` is a snapshot. Pattern groups and calibrators are copied before
runtime setup. Runtime edits to pattern groups or calibration are not supported.

If the controller needs more runtime data, extend `RuntimeContext`; do not add a
direct dependency on `TaskLocalization`.

## Setup API

```cpp
void configure(const TaskLocalizeConfig &config);
SetupResult setup(const RuntimeContext &context);
bool isValid() const;
```

`configure()` refreshes the internal `LocalizationSignalMapper`.

`setup()`:

1. Resets existing runtime bindings.
2. Stores the context snapshot.
3. Validates PLC, VisionOutput, and camera runner roles.
4. Connects PLC value updates to `handlePlcValues`.
5. Binds active camera, PLC, and VisionOutput role contexts.
6. Validates active pattern group and active camera calibration.
7. Requests device connects through runners.
8. Publishes ready outputs once all required roles are healthy.

`SetupResult::valid` is false if required role bindings, pattern group, or
calibration are missing.

## Runtime Inputs

```cpp
void execute();
void handlePlcValues(const QMap<QString, QVariant> &values);
void setActiveCameraNumber(int cameraNumber);
void setActivePatternGroupNumber(int patternGroupNumber);
```

`execute()` is a manual runtime trigger. It starts a cycle only from
`ReadyForTrigger`.

`handlePlcValues()` maps PLC tags into logical signal events. Important event
names:

- `nActiveCamera`
- `nActivePatternGroup`
- `bExecuteTrigger`

`bExecuteTrigger` is edge-triggered. Only a rising edge from false to true can
start a cycle, and only while the controller is ready.

`setActiveCameraNumber()` rejects changes while a cycle is running. Accepted
changes are handled by disconnecting the previous active camera runner,
binding the new runner, validating calibration, publishing `bCameraValid`, and
requesting connect through `CameraRunner`.

`setActivePatternGroupNumber()` rejects changes while a cycle is running and
publishes `bPatternValid` plus fault outputs if the new group is invalid.

## Matching API

```cpp
signals:
    void runtimeMatchingRequested(int cycleId,
                                  std::shared_ptr<mtc::MatchGroup> group,
                                  cv::Mat image);

public:
    void onRuntimeMatchingFinished(int cycleId, mtc::MatchResult matchResult);
```

The controller never runs matching directly. It emits
`runtimeMatchingRequested(...)` after a successful camera grab.

`TaskLocalization` connects this signal to the matching worker. The worker runs
`LocalizationPipeline::runMatch(...)` and queues
`onRuntimeMatchingFinished(cycleId, result)` back to the controller.

The controller ignores a matching result when:

- `cycleId` does not equal the current active cycle id.
- the controller is no longer in `Running`.

This makes late results harmless after aborts or recovery.

## Runtime Cycle State

Internal cycle states:

- `NotReady`
- `ReadyForTrigger`
- `Running`
- `WaitingTriggerReset`
- `Recovering`
- `Faulted`

The normal happy path:

1. `ReadyForTrigger`.
2. Rising `bExecuteTrigger`.
3. `Running`.
4. Publish cycle-start outputs.
5. Request camera single shot.
6. Receive grab frame.
7. Emit `runtimeMatchingRequested`.
8. Receive matching result.
9. Convert image coordinates to world coordinates through active calibrator.
10. Send positions through `VisionOutputRunner`.
11. Publish success outputs.
12. If trigger is still true, enter `WaitingTriggerReset`.
13. On trigger falling edge, clear `bMatchingFinished` and return ready.

Held trigger behavior is deliberate: a held true trigger must not enqueue a
second cycle.

## Output Coordinate Contract

`buildVisionOutputPositions()` converts each matched object from image
coordinates into world coordinates:

- `imageX/imageY` come from `MatchedObject::point_Center`.
- `imageR` comes from `MatchedObject::point_angle`.
- `world.x/y/z` comes from `calib::Calibrator::imageToRobot`.
- `world.r` comes from `calib::Calibrator::rotateImageToRobot`.

Objects with collision are marked as skipped and are not sent to
VisionOutput.

## Fault And Recovery

The controller publishes fault state with:

- `bTaskFault = true`
- `nFaultCode = <stable code>`
- `bTaskReady = false`
- `bMatchingBusy = false`
- `bMatchingFinished = true` for cycle faults

Cycle abort increments the active cycle id so late matching results are ignored.

Role recovery is policy based. Default policies retry recoverable statuses:

- `LostConnected`
- `ConnectFailed`

Default retry settings are:

- 10 retries
- 5000 ms interval

Recovering roles:

- camera
- primary PLC
- vision output

## Signals To Task/UI

```cpp
void signalChanged(QString name, QVariant value);
void cycleResultUpdated(CycleResult result);
void taskLogAppended(TaskLogEntry entry);
void runtimeCycleStarted(QString message);
void runtimeRecovering(QString message);
void runtimeReady(QString message);
void runtimeFault(QString message);
```

`signalChanged` is emitted for both incoming mapped PLC values and controller
published outputs. The dashboard and task-level signal monitor consume this.

`runtimeCycleStarted`, `runtimeRecovering`, `runtimeReady`, and `runtimeFault`
drive the `TaskLocalization` task state machine.

## PLC Output Publishing

`publishBoolSignal(name, value)` and `publishNumberSignal(name, value)`:

1. Emit `signalChanged(name, value)`.
2. Resolve the mapped PLC tag by signal name.
3. If a tag exists, request a PLC write through `PlcRunner`.

Empty PLC output tags are allowed. The task-facing signal still emits.

## Race Guards

The controller intentionally blocks ready transitions while `Running`. This
prevents delayed device `Connected` status events from overriding an active
cycle state.

Camera command failures only abort immediately on command timeout. Non-timeout
grab failures are mapped by `onCameraGrabFinished()` to
`CameraGrabTimeout` when the frame is missing or invalid.

