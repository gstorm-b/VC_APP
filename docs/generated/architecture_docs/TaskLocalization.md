# TaskLocalization Class Documentation

## Overview

`TaskLocalization` is a concrete implementation of `ITask` that performs image-based localization using template matching and camera I/O.

This task:
1. Acquires images from an assigned camera
2. Runs template matching to find patterns in the image
3. Sends match results (position, rotation, confidence) to assigned output devices
4. Coordinates with PLC devices for signal-based control
5. Manages a pipeline of execution stages (capture → match → output → PLC feedback)

A typical workflow involves operators configuring patterns, assigning cameras and PLC devices, then executing localization cycles in a runtime loop.

## Location

- **Header**: `src/model/task_localization.h`
- **Implementation**: `src/model/task_localization.cpp`
- **Namespace**: `vc::model`
- **Base Class**: `ITask` (which inherits from `QObject`)

## Dependencies

**Project Internal:**
- `model/itask.h` — Base interface
- `model/task_localization_config.h` — Configuration data structure
- `model/localization_pipeline.h` — Execution pipeline stages
- `model/localization_runtime_controller.h` — Runtime state machine
- `matching/pattern_group_manager.h` — Pattern/template management
- `device/camera/camera_device.h` — Camera interface
- `device/plc/mc_protocol_device.h` — PLC device
- `runtime/camera_runner.h` — Camera I/O runner
- `runtime/plc_runner.h` — PLC command runner

**External:**
- OpenCV (`cv::Mat`, `opencv2/imgcodecs.hpp`)

## Class Hierarchy

```
QObject
  └── ITask
       └── TaskLocalization (concrete)
```

Overrides all pure virtual methods from `ITask`.

## Constants

#### #### const int limit_comm_device = 1
Maximum number of PLC devices that can be assigned to this task: **1**.

#### #### const int limit_vision_output_device = 1
Maximum number of vision output devices (TCP/IP servers) that can be assigned: **1**.

#### #### const int limit_num_camera = 16
Maximum number of cameras that can be assigned: **16** (typically 1 owned + up to 15 shared).

## Constructor

#### #### TaskLocalization(QString name, QString id = "", QObject* parent = nullptr)
Constructs a localization task with a name and optional ID.

**Parameters:**
- `name` — Human-readable task name
- `id` — Optional unique task ID (generated if empty)
- `parent` — Qt parent object

**Initialization:**
- Initializes pattern manager (`m_patternManager`)
- Sets default device limits
- Validates basic configuration

## Destructor

#### #### ~TaskLocalization() override
Cleans up runtime controller and matching worker threads.

## Public Methods

### Overrides from ITask

#### #### TaskType taskType() const override
Returns `TaskType::LocalizationTask`.

#### #### bool isValid() const override
Checks if the task is ready to execute.

**Returns:**
- `true` if configuration is complete (camera, patterns, config)
- `false` if required setup is missing

#### #### bool isReachLimitOfDeviceType(vc::device::DeviceType t) const override
Checks if the task has reached its device limit for type `t`.

**Parameters:**
- `t` — Device type to check

**Returns:**
- `true` if already at limit for this type
- `false` otherwise

#### #### void beginRuntime(bool mergeToTaskThread = false) override
Starts the localization runtime:
1. Creates runtime controller
2. Spawns matching worker thread (unless `mergeToTaskThread == true`)
3. Wires all signal/slot connections
4. Queues initial setup commands

**Parameters:**
- `mergeToTaskThread` — If `true`, run matching on main thread; if `false`, use worker thread

#### #### void endRuntime() override
Stops the localization runtime:
1. Destroys runtime controller
2. Stops matching worker thread
3. Disconnects all signals/slots
4. Cleans up resources

#### #### void stopAll() override
Immediately halts all task operations (emergency stop). Used to abort in-progress localization cycles.

#### #### QJsonObject toJson() const override
Serializes the task to JSON, including:
- Task metadata (name, ID, state)
- Configuration (`TaskLocalizeConfig`)
- Pattern groups and parameters

**Returns:**
- A `QJsonObject` containing all task data

#### #### bool fromJson(const QJsonObject& obj) override
Deserializes the task from JSON (reverse of `toJson()`).

**Returns:**
- `true` if deserialization succeeded

#### #### QMap<QString, cv::Mat> getTaskImageMap() override
Returns a map of pattern images (templates).

**Key format:** `"g{groupNumber}_p{patternNumber}"` (e.g., `"g0_p1"`)

**Returns:**
- Map of image key → OpenCV matrix

#### #### bool loadTaskImageMap(QMap<QString, cv::Mat>& mapping) override
Loads image data into the pattern manager.

**Parameters:**
- `mapping` — Map of image key → matrix

**Returns:**
- `true` if all images loaded successfully

### Configuration & Management

#### #### void setTaskLocalizeConfig(const TaskLocalizeConfig& cfg)
Sets the localization configuration (parameters, thresholds, etc.).

#### #### TaskLocalizeConfig taskLocalizeConfig() const
Returns the current configuration.

#### #### mtc::PatternGroupManager* patternManager() const
Returns a pointer to the pattern manager for creating/editing patterns.

### Device Access

#### #### vc::device::PlcDevice* plcDevice() const
Returns the assigned PLC device (if any).

**Returns:**
- Pointer to the PLC device, or `nullptr` if not assigned

**Note:** For vendor-specific access (e.g., Mitsubishi MC frames), use:
```cpp
auto mcDevice = qobject_cast<McProtocolDevice*>(task->plcDevice());
```

### Pattern/Image Commissioning

#### #### void startCommissionMatching(std::shared_ptr<mtc::MatchGroup> group, cv::Mat& image)
Initiates template matching on a provided image and pattern group (typically for UI preview).

**Emits:** `startCommissionMatchingRequest(group, image.clone())` signal

**Usage:** Called by UI to preview pattern matching before running the task.

## Public Slots

#### #### void setupTask()
Prepares the task for execution:
1. Validates all devices are connected
2. Initializes pattern manager
3. Creates runtime controller
4. Wires signal/slot connections

Called automatically by `beginRuntime()`.

#### #### void executeLocalization()
Executes a single localization cycle:
1. Acquires image from camera
2. Runs template matching
3. Sends results to output devices
4. Waits for PLC feedback
5. Logs results

This is the main execution loop called repeatedly by the runtime.

#### #### void setCameraNumber(int number)
Sets the active camera number in the pattern manager.

#### #### void setPatternNumber(int number)
Sets the active pattern group number in the pattern manager.

## Private Slots

#### #### void onCommDeviceValueChanged(QMap<QString, QVariant> values)
Called when PLC device sends updated signal values.

#### #### void onSignalChangeCameraNumber(QVariant value)
Called when camera selection signal changes.

#### #### void onSignalChangePatternNumber(QVariant value)
Called when pattern selection signal changes.

#### #### void onRuntimeCycleStarted(const QString& message)
Called when a localization cycle begins. Typically updates UI status.

#### #### void onRuntimeRecovering(const QString& message)
Called when the task is recovering from an error.

#### #### void onRuntimeReady(const QString& message)
Called when the task is ready for the next cycle.

#### #### void onRuntimeFault(const QString& message)
Called when a fault occurs during execution.

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `startPLCRequest()` | — | PLC I/O cycle begins |
| `startCameraRequest()` | — | Camera capture begins |
| `startCommissionMatchingRequest(std::shared_ptr<mtc::MatchGroup>, cv::Mat)` | Group, image | Pattern commissioning starts |
| `commissionMatchingFinished(mtc::MatchResult)` | Result | Pattern matching completes |
| `cycleResultUpdated(CycleResult)` | Cycle result | Localization cycle finishes |
| `taskLogAppended(TaskLogEntry)` | Log entry | New log entry added |
| `cameraChanged(QString)` | Camera name | Active camera changes |
| `patternChanged(QString)` | Pattern name | Active pattern changes |

(Inherits signals from `ITask`: `nameChanged`, `devicesChanged`, `taskStateChanged`, etc.)

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `m_config` | `TaskLocalizeConfig` | Configuration parameters |
| `m_pipeline` | `LocalizationPipeline` | Execution pipeline stages |
| `m_runtimeController` | `LocalizationRuntimeController*` | State machine for execution |
| `m_patternManager` | `mtc::PatternGroupManager*` | Pattern/template storage |
| `m_plcDeviceId` | `QString` | ID of assigned PLC device |
| `m_matchingRunner` | `QThread*` | Worker thread for matching |
| `m_matchingWorker` | `QObject*` | Matching algorithm worker |
| `m_lastMatchResult` | `mtc::MatchResult` | Result from last cycle |
| `m_lastVisionOutput` | `QString` | Last output to vision server |
| `m_limitDeviceMap` | `QMap<DeviceType, int>` | Device count limits |
| `m_isValid` | `bool` | Whether task is configured |

## Qt Meta Type Registration

```cpp
Q_DECLARE_METATYPE(mtc::MatchResult)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(std::shared_ptr<mtc::MatchGroup>)
```

These declarations allow `MatchResult`, `cv::Mat`, and `MatchGroup` to be passed through Qt signals/slots.

## Execution Flow

```
User creates TaskLocalization
    ↓
User configures patterns, cameras, PLC
    ↓
task->beginRuntime()
    ├─ Creates LocalizationRuntimeController
    ├─ Spawns matching worker thread
    └─ Calls setupTask()
    ↓
User calls task->executeLocalization() (in a loop)
    ├─ Camera captures image
    ├─ Pattern matching runs
    ├─ Results sent to output devices
    ├─ PLC feedback received
    └─ Emits cycleResultUpdated()
    ↓
User calls task->endRuntime()
    ├─ Destroys runtime controller
    ├─ Stops worker thread
    └─ Cleans up resources
```

## Usage Example

```cpp
// Create task
auto task = new vc::model::TaskLocalization("Localize Assembly Part");

// Configure
vc::model::TaskLocalizeConfig cfg;
cfg.matchThreshold = 0.75;
cfg.maxPatterns = 5;
task->setTaskLocalizeConfig(cfg);

// Assign devices
task->assignDevice("camera_001");
task->assignDevice("plc_mitsubishi_001");
task->assignDevice("vision_output_tcp_001");

// Add patterns via pattern manager
auto patternMgr = task->patternManager();
cv::Mat templateImage = cv::imread("pattern.png");
patternMgr->addPattern("Part A", templateImage);

// Begin execution
task->beginRuntime();

// Execute cycles
for (int i = 0; i < 100; ++i) {
    task->executeLocalization();
    QCoreApplication::processEvents();  // Let events process
}

// Cleanup
task->endRuntime();
```

## Threading Model

- **Main thread**: All property access, configuration, serialization
- **Worker thread 1**: Matching algorithm (spawned by `beginRuntime()`)
- **Worker threads 2+**: Device runners (camera I/O, PLC requests)

## Related Classes

- **ITask** (`model/itask.h`) — Base interface
- **TaskLocalizeConfig** (`model/task_localization_config.h`) — Configuration
- **LocalizationRuntimeController** (`model/localization_runtime_controller.h`) — Runtime state
- **PatternGroupManager** (`matching/pattern_group_manager.h`) — Pattern storage
- **ImageMatcher** (`matching/image_matcher.h`) — Template matching algorithm
- **Project** (`model/project.h`) — Container for tasks

