# LocalizationRuntimeController Class Documentation

## Overview

`LocalizationRuntimeController` is the state machine and orchestrator for localization task execution. It coordinates:
- Camera image acquisition
- Pattern matching with templates
- Output device communication
- PLC signal handling
- Error recovery and fault codes
- Cycle result aggregation

A single instance is created per `TaskLocalization` when `beginRuntime()` is called, and destroyed when `endRuntime()` is called.

## Location

- **Header**: `src/model/localization_runtime_controller.h`
- **Implementation**: `src/model/localization_runtime_controller.cpp`
- **Namespace**: `vc::model`
- **Base Class**: `QObject`

## Dependencies

**Qt Modules:**
- QtCore (QObject, QHash, QDateTime, etc.)

**External:**
- OpenCV (`cv::Mat`)

**Project Internal:**
- `device/camera/camera_device.h` — Camera interface
- `device/output_device/vision_output_request.h` — Output messages
- `matching/image_matcher.h` — Template matching engine
- `model/localization_fault_code.h` — Fault enumeration
- `model/localization_recovery_policy.h` — Error recovery
- `model/localization_signal_mapper.h` — Signal mapping
- `model/task_localization_config.h` — Configuration
- `runtime/device_command.h` — Device commands
- `runtime/camera_runner.h` — Camera execution
- `runtime/plc_runner.h` — PLC execution
- `calibration/calibrator.h` — Camera calibration

## Nested Structures & Types

### RuntimeContext

Configuration and state for the runtime controller.

```cpp
struct RuntimeContext {
    TaskLocalizeConfig config;
    QString primaryPlcDeviceId;
    QString visionOutputDeviceId;
    QPointer<vc::runtime::PlcRunner> primaryPlcRunner;
    QPointer<vc::runtime::VisionOutputRunner> visionOutputRunner;
    QMap<int, QString> cameraDeviceIds;                           // cam# → device ID
    QMap<int, QPointer<vc::runtime::CameraRunner>> cameraRunners; // cam# → runner
    QMap<int, std::shared_ptr<mtc::MatchGroup>> patternGroups;    // group# → patterns
    QMap<int, calib::Calibrator> cameraCalibrators;               // cam# → calibration
    int activeCameraNumber{-1};
    int activePatternGroupNumber{-1};
};
```

**Purpose:** Holds all runtime dependencies and state passed to `setup()`.

### CycleResult

Result of a single localization cycle.

```cpp
struct CycleResult {
    bool faulted{false};
    LocalizationFaultCode faultCode{LocalizationFaultCode::None};
    int detectedNumber{0};              // Patterns matched
    int sentNumber{0};                  // Results sent to output
    double matchingTimeMs{0.0};         // Matching execution time
    bool lowArea{false};                // Image area below threshold?
    cv::Mat displayImage;               // Annotated image for UI
    QVector<ResultRow> rows;            // One row per detected pattern
};
```

**Purpose:** Aggregates all results from a single execution cycle for UI display and logging.

### ResultRow

A single row in the cycle result (one detected pattern).

```cpp
struct ResultRow {
    int index{0};
    QString patternName;
    double score{0.0};
    double imageX{0.0};                 // Pixel coordinates
    double imageY{0.0};
    double imageR{0.0};                 // Rotation (degrees)
    vc::device::VisionOutputPosition world;  // Calibrated world coordinates
    QString status;                     // "OK", "OutOfBounds", etc.
};
```

### TaskLogEntry

A single log entry for UI display.

```cpp
struct TaskLogEntry {
    QDateTime timestamp;
    QString severity;                   // "INFO", "WARN", "ERROR"
    QString message;
};
```

### SetupResult

Result of the `setup()` validation.

```cpp
struct SetupResult {
    bool valid{false};
    QString primaryPlcDeviceId;
    QString visionOutputDeviceId;
    QStringList errors;
};
```

## Constructor & Destructor

#### #### LocalizationRuntimeController(QObject* parent = nullptr)
Constructs a controller in unconfigured state.

**Parameters:**
- `parent` — Qt parent object

#### #### ~LocalizationRuntimeController()
Destructor; cleans up state.

## Public Methods

### Configuration

#### #### void configure(const TaskLocalizeConfig& config)
Sets the task configuration (parameters, thresholds, etc.).

**Parameters:**
- `config` — Localization configuration

#### #### void setActiveCameraNumber(int cameraNumber)
Sets which camera is used for matching.

**Parameters:**
- `cameraNumber` — Camera index (0-based)

**Side Effects:**
- Subsequent `execute()` calls will use this camera

#### #### void setActivePatternGroupNumber(int patternGroupNumber)
Sets which pattern group is used for matching.

**Parameters:**
- `patternGroupNumber` — Pattern group index

**Side Effects:**
- Subsequent `execute()` calls will match against these patterns

#### #### void setRecoveryPolicies(const LocalizationRecoveryPolicy& cameraPolicy, const LocalizationRecoveryPolicy& plcPolicy, const LocalizationRecoveryPolicy& visionOutputPolicy)
Configures error recovery behavior for each device type.

**Parameters:**
- `cameraPolicy` — Recovery strategy for camera errors
- `plcPolicy` — Recovery strategy for PLC errors
- `visionOutputPolicy` — Recovery strategy for output device errors

**Note:** Recovery policies define how the controller handles failures (retry, abort, log, etc.).

### Setup & Execution

#### #### SetupResult setup(const RuntimeContext& context)
Validates and initializes the runtime context.

**Parameters:**
- `context` — Runtime configuration with device runners and pattern groups

**Returns:**
- `SetupResult` containing:
  - `valid` — `true` if all validations passed
  - `primaryPlcDeviceId` — ID of primary PLC device
  - `visionOutputDeviceId` — ID of vision output device
  - `errors` — List of validation errors (if any)

**Validation checks:**
- At least one camera assigned
- PLC device configured (if required)
- Pattern groups are non-empty
- Camera calibration data available (if needed)

**When to call:**
- After creating the controller and before calling `execute()`
- Typically called from `TaskLocalization::setupTask()`

#### #### void execute()
Executes a single localization cycle.

**Execution sequence:**
1. Acquire image from active camera
2. Apply calibration if needed
3. Run template matching
4. Filter results by bounding box and thresholds
5. Send matched results to output device
6. Receive PLC feedback (if configured)
7. Build `CycleResult` and emit signals

**Emits:**
- `cycleStarted()` signal at cycle start
- `cycleFinished(CycleResult)` signal at cycle end (even on failure)
- `logAppended(TaskLogEntry)` for each log entry
- Fault-specific signals on error

**When to call:**
- In a loop during task execution
- Typically called from `TaskLocalization::executeLocalization()`

**Example:**
```cpp
for (int i = 0; i < 100; ++i) {
    controller->execute();
    QCoreApplication::processEvents();
}
```

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `cycleStarted()` | — | New cycle begins |
| `cycleFinished(CycleResult)` | Result | Cycle completes (success or fault) |
| `logAppended(TaskLogEntry)` | Log entry | New log entry available |
| `faultOccurred(LocalizationFaultCode)` | Fault code | Fault detected |
| `recoveryAttempting(QString)` | Recovery message | Attempting error recovery |
| `recoverySucceeded(QString)` | Message | Recovery successful |
| `recoveryFailed(QString)` | Message | Recovery failed |

## Enumerations (Via Dependencies)

### LocalizationFaultCode

Enumeration of fault conditions that can occur during execution.

**Values (examples):**
- `None` — No fault
- `CameraNotConnected` — Camera connection lost
- `CameraGrabFailed` — Image capture failed
- `MatchingFailed` — Template matching error
- `NoMatchFound` — No patterns detected in image
- `PlcCommunicationFailed` — PLC communication error
- `OutputDeviceFailed` — Vision output server error
- `CalibrationMissing` — Camera calibration not available
- `LowConfidence` — All matches below threshold

(See `model/localization_fault_code.h` for complete list)

## Usage Example

```cpp
// Create controller
auto controller = new LocalizationRuntimeController();

// Configure
vc::model::TaskLocalizeConfig cfg;
cfg.matchThreshold = 0.75;
cfg.maxResults = 5;
controller->configure(cfg);

// Set active camera and patterns
controller->setActiveCameraNumber(0);
controller->setActivePatternGroupNumber(0);

// Set recovery policies
vc::model::LocalizationRecoveryPolicy cameraPolicy;
cameraPolicy.retryCount = 3;
cameraPolicy.retryDelayMs = 500;
controller->setRecoveryPolicies(cameraPolicy, {}, {});

// Build runtime context
vc::model::LocalizationRuntimeController::RuntimeContext context;
context.config = cfg;
context.primaryPlcDeviceId = "plc_001";
context.visionOutputDeviceId = "vision_output_001";
context.activeCameraNumber = 0;
context.activePatternGroupNumber = 0;
// ... fill in runners, patterns, calibrators ...

// Setup
auto setupResult = controller->setup(context);
if (!setupResult.valid) {
    qWarning() << "Setup failed:" << setupResult.errors;
    return;
}

// Connect signals
connect(controller, &LocalizationRuntimeController::cycleFinished,
        this, [this](const auto& result) {
    qDebug() << "Cycle result: detected" << result.detectedNumber 
             << "sent" << result.sentNumber;
    if (result.faulted) {
        qWarning() << "Fault code:" << (int)result.faultCode;
    }
});

// Execute cycles
for (int i = 0; i < 100; ++i) {
    controller->execute();
    QCoreApplication::processEvents();
}
```

## Threading Model

- **Main thread**: All configuration and setup
- **Worker threads**: Device runners (camera, PLC, vision output)
- **Signal/slot connections**: `Qt::QueuedConnection` for thread safety

## Execution State Machine

```
Idle
  ↓
[setup()] → Setup validation
  ├─ Valid → Ready
  └─ Invalid → Error
  ↓
Ready
  ├─→ [execute()]
  │    ├─ Grab image
  │    ├─ Match patterns
  │    ├─ Send results
  │    ├─ Receive feedback
  │    └─ Emit cycleFinished()
  │    ↓
  └─→ Ready (repeat)
```

## Error Recovery

The controller implements three-level error recovery:

1. **Retry**: Automatically retry the failed operation (configurable count)
2. **Recovery**: Call recovery handler if available
3. **Fault**: If recovery fails, emit fault signal and log error

Recovery policies are customizable per device type.

## Related Classes

- **TaskLocalization** (`model/task_localization.h`) — Creates and owns controller
- **ImageMatcher** (`matching/image_matcher.h`) — Template matching engine
- **CameraRunner** / **PlcRunner** / **VisionOutputRunner** (`runtime/`) — Device I/O
- **LocalizationRecoveryPolicy** (`model/localization_recovery_policy.h`) — Error handling
- **LocalizationFaultCode** (`model/localization_fault_code.h`) — Fault enumeration
- **Calibrator** (`calibration/calibrator.h`) — Camera calibration

