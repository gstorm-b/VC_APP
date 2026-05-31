# TaskLocalization API

`TaskLocalization` is the facade between the project/task framework and the
localization runtime implementation.

Source:

- `src/model/task_localization.h`
- `src/model/task_localization.cpp`

## Responsibility

`TaskLocalization` owns:

- The task identity and base `ITask` lifecycle.
- Persistent `TaskLocalizeConfig`.
- Assigned-device limits for PLC, VisionOutput, and Camera devices.
- Pattern group persistence through `PatternGroupManager`.
- Pattern image BLOB export/import for `ProjectRepository`.
- The `LocalizationPipeline` instance used by commission and runtime matching.
- The matching worker thread.
- Creation, destruction, and signal wiring of `LocalizationRuntimeController`.

`TaskLocalization` does not own runtime device I/O details. Runtime device
connect, disconnect, single-shot, PLC writes, and vision-output sends are routed
through the runtime controller and device runners.

## Public Surface

### Lifecycle

```cpp
void beginRuntime(bool mergeToTaskThread = false) override;
void endRuntime() override;
void stopAll() override;
```

`beginRuntime(false)` is the supported runtime mode for localization. If
`mergeToTaskThread` is passed as true, localization logs a warning and keeps
per-device threads.

Runtime startup order:

1. Transition task state to `RuntimeStarting`.
2. Sync task runners with assigned devices.
3. Enter `TaskRunner` runtime in per-device-thread mode.
4. Create the runtime controller if needed.
5. Move the controller to `TaskRunner::runtimeThread()`.
6. Build `RuntimeContext`.
7. Setup the controller using a blocking queued call when needed.
8. Enter `Ready` only after runtime setup succeeds and required roles become
   healthy.

`endRuntime()` and `stopAll()` destroy the runtime controller before entering
idle, then recreate a fresh controller in the task thread for the next session.

### Configuration

```cpp
void setTaskLocalizeConfig(const TaskLocalizeConfig &cfg);
TaskLocalizeConfig taskLocalizeConfig() const;
```

`setTaskLocalizeConfig()` updates the persistent config and queues
`LocalizationRuntimeController::configure()` when a controller exists.

`TaskLocalizeConfig` contains PLC tag names and device role bindings. See
[plc_signal_contract.md](plc_signal_contract.md).

### Pattern Management

```cpp
mtc::PatternGroupManager *patternManager() const;
void startCommissionMatching(std::shared_ptr<mtc::MatchGroup> group,
                             cv::Mat &image);
```

Commission matching is asynchronous. `startCommissionMatching()` clones the
image and emits `startCommissionMatchingRequest(...)`. The matching worker runs
`LocalizationPipeline::runMatch(...)` and emits `commissionMatchingFinished`.

Runtime matching uses the same worker thread but is initiated by the runtime
controller through `runtimeMatchingRequested(...)`.

### Runtime Commands

```cpp
public slots:
    void setupTask();
    void executeLocalization();
    void setCameraNumber(int number);
    void setPatternNumber(int number);
```

`setupTask()` builds the runtime context and calls controller setup.

`executeLocalization()` queues `LocalizationRuntimeController::execute()`.
Manual execution is accepted only when the task is in `Ready` or
`RunningCycle`.

`setCameraNumber()` only validates that the logical camera number maps to an
assigned camera device. It does not call `CameraDevice::deviceConnect()` or
`CameraDevice::deviceDisconnect()`. The accepted request is queued to
`LocalizationRuntimeController::setActiveCameraNumber()`.

`setPatternNumber()` queues
`LocalizationRuntimeController::setActivePatternGroupNumber()`.

### Signals

```cpp
void commissionMatchingFinished(mtc::MatchResult result);
void cycleResultUpdated(LocalizationRuntimeController::CycleResult result);
void taskLogAppended(LocalizationRuntimeController::TaskLogEntry entry);
```

`cycleResultUpdated` drives dashboard result views. `taskLogAppended` is a
task-scoped operator log stream.

`TaskLocalization` also forwards controller signal changes through the base
`ITask::signalChanged` signal.

## RuntimeContext Construction

`TaskLocalization::buildRuntimeContext()` snapshots everything the runtime
controller needs:

- copied `TaskLocalizeConfig`
- primary PLC device id and `PlcRunner`
- vision-output device id and `VisionOutputRunner`
- camera number to device id map
- camera number to `CameraRunner` map
- camera number to camera calibrator map
- pattern group snapshots
- active camera number
- active pattern group number

The controller must not call back into `TaskLocalization` to resolve runtime
data after setup.

## Threading Model

During runtime:

- `TaskLocalization` stays in the task/object owner thread.
- `LocalizationRuntimeController` lives in `TaskRunner::runtimeThread()`.
- Camera, PLC, and VisionOutput devices stay on their per-device threads.
- Matching runs on `TaskLocalization::matchingRunner`.

All task-to-controller runtime calls must be queued. Controller-to-task
notifications are Qt signals connected as queued connections.

## Persistence API

```cpp
QJsonObject toJson() const override;
bool fromJson(const QJsonObject &obj) override;
QMap<QString, cv::Mat> getTaskImageMap() override;
bool loadTaskImageMap(QMap<QString, cv::Mat> &mapping) override;
```

JSON persistence is delegated:

- Base task state and config are handled by `ITask`.
- Pattern library schema is handled by `PatternGroupManager`.
- Pattern images are stored separately as BLOBs by `ProjectRepository`.

Pattern image keys use the stable format:

```text
g{groupNumber}_p{patternNumber}
```

Do not key image storage by group or pattern name; names are user-editable.

## Maintenance Notes

- Do not add runtime calls from `TaskLocalization` directly to concrete devices.
  Route through runners and the controller.
- Do not let `LocalizationRuntimeController` call `TaskLocalization` methods.
  Add fields to `RuntimeContext` if the controller needs more runtime data.
- If a new task-facing runtime signal is added, wire it through both controller
  and dashboard/settings as needed.
- If lifecycle or ownership changes, update `uml/03_runtime_threading.puml` and
  `uml/04_localization_task.puml`.

