# ITask Interface Documentation

## Overview

`ITask` is the abstract base class for all task types in the NCR picking system. It defines the contract that all task implementations must follow, including task identification, lifecycle management, device assignment, camera ownership, state machine integration, and serialization.

A task represents a single operation or workflow that the picking system executes (e.g., localization, pick & place). Multiple tasks can be created within a single project and assigned to different devices.

## Location

- **Header**: `src/model/itask.h`
- **Implementation**: `src/model/itask.cpp`
- **Namespace**: `vc::model`
- **Base Class**: `QObject` (Qt Object Model with signals/slots)

## Dependencies

**Qt Modules:**
- QtCore (QObject, QString, QJsonObject, QUuid)

**Project Internal:**
- `task_define.h` — Task type and state enumerations
- `task_state_machine.h` — State machine logic
- `itask_config.h` — Task configuration interface
- `runtime/task_runner.h` — Task execution runner
- `device/idevice.h` — Device interface
- `logger/app_logger.h` — Logging utilities

**External:**
- OpenCV (cv::Mat for image data)

## Class Hierarchy

```
QObject
  └── ITask (abstract)
       └── TaskLocalization (concrete implementation)
```

`ITask` is pure virtual and must be subclassed. Subclasses override `taskType()` and `isValid()`.
`TaskType::PickPlaceTask` is reserved in the enum, but the old `PickPlaceTask`
header is currently commented out and is not an active concrete task.

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `name` | `QString` | `name()` | `setName()` | `nameChanged` | Human-readable task name |

## Pure Virtual Methods (Must Override)

#### TaskType taskType() const = 0
Returns the concrete task type (e.g., `TaskType::LocalizationTask`).

**Returns:**
- A `TaskType` enum value identifying this task's type

#### bool isValid() const = 0
Validates that the task is in a valid, executable state.

**Returns:**
- `true` if the task can be run
- `false` if required configuration is missing

## Public Getters & Setters

### Identification & Naming

#### QString id() const
Returns the unique identifier for this task. Generated at construction, immutable.

#### QString name() const
Returns the human-readable task name.

#### void setName(const QString& v)
Sets the task name and emits `nameChanged()`.

### Task State

#### TaskState taskState() const
Returns the current execution state of the task.

**Returns:**
- One of: `Idle`, `CommissionStarting`, `Commission`, `RuntimeStarting`,
  `Ready`, `RunningCycle`, `Recovering`, `Faulted`, `Stopping`

The legal transitions are defined in `task_state_machine.h`. The common paths
are:

- Commission: `Idle -> CommissionStarting -> Commission -> Stopping -> Idle`
- Runtime: `Idle -> RuntimeStarting -> Ready -> RunningCycle -> Ready`
- Recovery/fault: runtime states may move through `Recovering` or `Faulted`;
  `Faulted` must move through `Stopping` before returning to `Idle`.

#### QString taskStateName() const
Returns the human-readable name of the current task state.

### Camera Ownership

A task can own a camera exclusively or share it with other tasks.

#### CameraSourceType cameraSourceType() const
Returns how this task accesses the camera.

**Returns:**
- `Source_Owned` — Camera is exclusively owned by this task
- `Source_Shared` — Camera is shared among multiple tasks

#### void setCameraSourceType(CameraSourceType v)
Sets the camera ownership mode and emits `cameraSourceTypeChanged()`.

#### bool isOwned() const
Convenience method: returns `true` if `cameraSourceType() == Source_Owned`.

#### bool isShared() const
Convenience method: returns `true` if `cameraSourceType() == Source_Shared`.

#### QString ownedCameraId() const
Returns the ID of the camera assigned to this task (if owned).

#### void setOwnedCameraId(const QString& v)
Sets the camera ID for an owned camera.

### Device Assignment

A task can be bound to multiple devices (e.g., camera + PLC + robot).

#### void assignDevice(const QString& deviceId)
Adds a device to this task. No-op if device is already assigned.

**Emits:** `devicesChanged()`

#### void unassignDevice(const QString& deviceId)
Removes a device from this task.

**Emits:** `devicesChanged()`

#### QStringList assignedDeviceIds() const
Returns all assigned device IDs.

## Virtual Methods (Override Optional)

#### void beginCommission()
Starts the commission phase. The base implementation transitions through
`CommissionStarting`, synchronizes runners for assigned devices, asks
`TaskRunner` to enter commission mode, transitions to `Commission`, and emits
`commissionStarted()`.

#### void endCommission()
Stops the commission phase. The base implementation transitions through
`Stopping`, returns `TaskRunner` to idle, transitions to `Idle`, and emits
`commissionStopped()`.

#### void beginRuntime(bool mergeToTaskThread = false)
Starts production runtime. The base implementation transitions through
`RuntimeStarting`, synchronizes runners for assigned devices, asks `TaskRunner`
to enter runtime mode, transitions to `Ready`, and emits `runtimeStarted()`.

**Parameters:**
- `mergeToTaskThread` — requests a single runtime thread for all devices when
  supported; the default keeps per-device runner threads.

`TaskLocalization` overrides this to keep per-device runner threads and move its
`LocalizationRuntimeController` onto the runtime thread.

#### void endRuntime()
Stops production runtime. The base implementation transitions through
`Stopping`, returns `TaskRunner` to idle, transitions to `Idle`, and emits
`runtimeStopped()`.

#### void stopAll()
Stops all task activity regardless of the current phase and returns the task to
`Idle` when the transition is legal.

#### QMap<QString, cv::Mat> getTaskImageMap()
Returns a map of image keys to OpenCV matrices (e.g., template patterns). Used for serialization.

#### bool loadTaskImageMap(QMap<QString, cv::Mat>& mapping)
Loads image data into the task (e.g., when deserializing a project).

**Returns:**
- `true` if load succeeded

#### QJsonObject toJson() const
Serializes the task to JSON, including metadata, configuration, and state.

**Returns:**
- A `QJsonObject` containing all task data

#### bool fromJson(const QJsonObject& obj)
Deserializes the task from JSON.

**Parameters:**
- `obj` — JSON object previously created by `toJson()`

**Returns:**
- `true` if deserialization succeeded

#### bool isReachLimitOfDeviceType(vc::device::DeviceType t) const
Checks if the task has already reached its limit for a given device type.

**Parameters:**
- `t` — Device type (e.g., `DeviceType::Camera`, `DeviceType::PLC`)

**Returns:**
- `true` if the task already has the maximum allowed devices of type `t`
- `false` otherwise

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `nameChanged()` | — | Task name is modified |
| `configChanged()` | — | Task configuration pointer changes |
| `cameraSourceTypeChanged()` | — | Camera ownership mode changes |
| `patternsChanged()` | — | Pattern data changes |
| `devicesChanged()` | — | Assigned devices list changes |
| `taskStateChanged(TaskState)` | New state | Task execution state transitions |
| `signalChanged(QString, QVariant)` | Signal name/value | Runtime signal values are forwarded to the UI |
| `commissionStarted()` / `commissionStopped()` | — | Commission phase starts/stops |
| `runtimeStarted()` / `runtimeStopped()` | — | Runtime phase starts/stops |

## Constructor

#### ITask(QString init_id = "", QObject* parent = nullptr)
Constructs a task with optional initialization ID.

**Parameters:**
- `init_id` — If non-empty, use as the task ID; otherwise, generate a UUID
- `parent` — Qt parent object

**Note:** The task ID is immutable after construction. If `init_id` is empty, a new UUID is generated via `QUuid::createUuid()`.

## Destructor

#### virtual ~ITask() = default
Virtual destructor for proper cleanup in subclasses.

## Usage Example

```cpp
// Create a concrete task (TaskLocalization is a subclass of ITask)
auto task = new vc::model::TaskLocalization("Locate Part Assembly");

// Set camera ownership
task->setCameraSourceType(vc::model::CameraSourceType::Source_Owned);
task->setOwnedCameraId("camera_001");

// Assign devices
task->assignDevice("camera_001");
task->assignDevice("plc_mitsubishi_001");

// Serialize
QJsonObject json = task->toJson();

// Begin execution
task->beginRuntime();  // Enters runtime and prepares assigned-device runners
// ... task runs ...
task->endRuntime();    // Cleans up
```

## Concrete Implementations

### TaskLocalization
Implements image-based localization using template matching and camera I/O.

### PickPlaceTask
Reserved/stubbed only. `src/model/pick_and_place_task.h` is commented out and is
not registered as an active concrete task.

## Threading Model

- **Main/UI thread**: Task properties, JSON serialization, and UI-facing signals.
- **TaskRunner**: Owns per-device runners and phase transitions for commission
  and runtime.
- **Device runner threads**: Camera, PLC, and VisionOutput runners mediate
  device work off the UI thread.
- **Localization matching worker**: `TaskLocalization` owns a dedicated matching
  thread for commission matching and runtime matching callbacks.

## Related Classes

- **Project** (`model/project.h`) — Container for tasks
- **IDevice** (`device/idevice.h`) — Interface for assigned devices
- **TaskRunner** (`runtime/task_runner.h`) — Executes task logic
- **TaskFactory** (`model/task_factory.h`) — Creates task instances

