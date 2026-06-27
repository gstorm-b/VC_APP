# TaskRunner Class Documentation

## Overview

`TaskRunner` is the central manager for all device threads belonging to a single task. It orchestrates the lifecycle of device runners and manages the three-phase execution model.

Responsibilities:
- Register/unregister device runners
- Manage device threads and the runtime coordinator thread
- Transition between three phases: Idle, Commission, Runtime
- Route device operations to the appropriate runner

A `TaskRunner` instance is owned by each `ITask` and coordinates all device I/O for that task.

## Location

- **Header**: `src/runtime/task_runner.h`
- **Implementation**: `src/runtime/task_runner.cpp`
- **Namespace**: `vc::runtime`
- **Base Class**: `QObject`

## Lifecycle Phases

The task runner manages three distinct phases:

### 1. Idle Phase
- **State**: No threads running; all devices live on the main (GUI) thread
- **Purpose**: Configuration and setup before execution
- **Entry**: `enterIdle()` or initial state after construction
- **Typical duration**: Entire application lifetime except during device operations

### 2. Commission Phase
- **State**: Each registered device gets its own `HighPriority QThread`
- **Purpose**: Test device connections, retrieve status, and preview operations
- **Entry**: `enterCommission()`
- **Exit**: `enterRuntime()` or `enterIdle()`
- **Typical duration**: Brief testing period before task execution
- **Example operations**: Connect camera, grab test image, verify PLC

### 3. Runtime Phase
- **State**: Devices remain in dedicated threads (or merge into one runtime thread)
- **Purpose**: Execute task operations with minimal latency
- **Entry**: `enterRuntime(bool mergeToTaskThread)`
- **Exit**: `enterIdle()`
- **Typical duration**: Active task execution
- **Two variants**:
  - **mergeToTaskThread=false** (recommended): Per-device threads + runtime coordinator thread
  - **mergeToTaskThread=true**: All devices in one runtime thread (sequential I/O)

## Public Members

#### #### enum class Phase
Enumerates the three execution phases.

**Values:**
- `Idle` — No threads running
- `Commission` — Per-device threads active; each device has a dedicated thread
- `Runtime` — Task execution phase with optional runtime coordinator thread

## Constructor & Destructor

#### #### TaskRunner(QObject* parent = nullptr)
Constructs a task runner in Idle phase.

**Parameters:**
- `parent` — Qt parent object

**Initial State:**
- No runners registered
- No threads created
- Phase = `Idle`

#### #### ~TaskRunner() override
Destructor; cleans up all device threads and runners.

**Cleanup:**
- Ensures all threads are stopped
- Destroys all device runners
- Deallocates runtime thread if present

## Public Methods

### Device Registration

#### #### void registerDevice(const QString& deviceId, std::shared_ptr<vc::device::IDevice> device)
Registers a device to be managed by this runner.

**Parameters:**
- `deviceId` — Unique device identifier
- `device` — Shared pointer to the device instance

**Side Effects:**
- Creates an appropriate `IDeviceRunner` subclass based on device type
- Stores runner in internal map
- Runner state synced with current phase (e.g., creates thread if in Commission phase)

**When to call:**
- During task setup to bind devices to task
- Typically called after `ITask::assignDevice()`

#### #### void unregisterDevice(const QString& deviceId)
Unregisters a device and stops its runner.

**Parameters:**
- `deviceId` — Device ID to unregister

**Side Effects:**
- Stops the device runner
- Moves device back to main thread if applicable
- Removes runner from internal map

**When to call:**
- When removing a device from the task

### Device Lookup

#### #### IDeviceRunner* runnerFor(const QString& deviceId) const
Retrieves the runner for a device.

**Parameters:**
- `deviceId` — Device ID

**Returns:**
- Pointer to the `IDeviceRunner`, or `nullptr` if device not registered

**Usage:**
```cpp
auto runner = taskRunner->runnerFor(cameraId);
if (runner) {
    auto cameraRunner = dynamic_cast<CameraRunner*>(runner);
    cameraRunner->requestGrabImage(...);
}
```

#### #### bool hasRunner(const QString& deviceId) const
Checks if a device has a registered runner.

**Parameters:**
- `deviceId` — Device ID

**Returns:**
- `true` if device is registered
- `false` otherwise

#### #### QStringList registeredDeviceIds() const
Returns the IDs of all registered devices.

**Returns:**
- List of device IDs

### Phase Transitions

#### #### void enterCommission()
Transitions to Commission phase.

**Effects:**
- Creates a `HighPriority QThread` for each registered device
- Moves devices to their respective threads
- Emits `phaseChanged(Phase::Commission)`
- Runners can now accept requests

**When to call:**
- Before testing device connections
- After all devices are registered

**Example:**
```cpp
taskRunner->registerDevice("camera_001", camera);
taskRunner->registerDevice("plc_001", plc);
taskRunner->enterCommission();
// Now can request operations on devices
```

#### #### void enterRuntime(bool mergeToTaskThread = false)
Transitions to Runtime phase.

**Parameters:**
- `mergeToTaskThread` — If `true`, merge all devices into one thread; if `false` (default), keep per-device threads

**Effects:**
- If `mergeToTaskThread=false`:
  - Creates a runtime coordinator thread (`m_runtimeThread`)
  - Keeps per-device threads
  - Task logic runs on runtime thread, coordinates with device threads
- If `mergeToTaskThread=true`:
  - Detaches all devices from per-device threads
  - Moves devices into single runtime thread
  - Simpler but sequential execution
- Emits `phaseChanged(Phase::Runtime)`

**When to call:**
- When ready to execute the main task logic
- After `enterCommission()` and device setup

**Example:**
```cpp
taskRunner->enterCommission();
// ... test devices ...
taskRunner->enterRuntime(false);  // Recommended: async per-device I/O
// Task execution loop begins
```

#### #### void enterIdle()
Transitions to Idle phase.

**Effects:**
- Stops all device threads
- Moves all devices back to main thread
- Destroys runtime thread if present
- Emits `phaseChanged(Phase::Idle)`

**When to call:**
- When task execution completes
- Before task is destroyed

### Phase Query

#### #### Phase currentPhase() const
Returns the current execution phase.

**Returns:**
- One of: `Idle`, `Commission`, `Runtime`

#### #### QThread* runtimeThread() const
Returns the runtime coordinator thread (if in Runtime phase).

**Returns:**
- Pointer to runtime thread, or `nullptr` if not in Runtime phase or `mergeToTaskThread=true`

**Usage:**
```cpp
if (taskRunner->currentPhase() == TaskRunner::Phase::Runtime) {
    auto runtimeThread = taskRunner->runtimeThread();
    if (runtimeThread) {
        qDebug() << "Runtime on thread:" << runtimeThread->objectName();
    }
}
```

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `phaseChanged(Phase)` | New phase | Phase transition completes |
| `requestDetach(QThread*)` | Thread pointer | Device detach requested |

## Private Methods

#### #### IDeviceRunner* createRunner(std::shared_ptr<vc::device::IDevice> device)
Factory method to create the appropriate runner type based on device type.

**Parameters:**
- `device` — Device instance

**Returns:**
- Newly allocated `IDeviceRunner` subclass (caller does not own; stored in map)

**Device → Runner Mapping:**
| Device Type | Runner |
|------------|--------|
| Camera | `CameraRunner` |
| PLC | `PlcRunner` |
| Robot | `RobotRunner` |
| Vision Output | `VisionOutputRunner` |

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `m_phase` | `Phase` | Current execution phase |
| `m_runtimeThread` | `QThread*` | Coordinator thread (Runtime phase only) |
| `m_runners` | `QMap<QString, IDeviceRunner*>` | Device ID → runner mapping |

## Usage Example

```cpp
// Create and initialize task runner
auto taskRunner = new TaskRunner();

// Register devices
auto camera = deviceMgr->deviceById("camera_001");
auto plc = deviceMgr->deviceById("plc_001");
taskRunner->registerDevice("camera_001", camera);
taskRunner->registerDevice("plc_001", plc);

// Commission phase: test devices
taskRunner->enterCommission();
qDebug() << "Phase:" << (int)taskRunner->currentPhase();

// Test camera connection
auto cameraRunner = qobject_cast<CameraRunner*>(taskRunner->runnerFor("camera_001"));
if (cameraRunner) {
    cameraRunner->requestConnect();
    // Wait for result via signal
}

// Move to runtime phase
taskRunner->enterRuntime(false);  // Per-device threads

// Execute task operations
auto cameraRunner = qobject_cast<CameraRunner*>(taskRunner->runnerFor("camera_001"));
cameraRunner->requestGrabImage(...);

// Transition back to idle
taskRunner->enterIdle();
```

## Threading Architecture

```
Main Thread (GUI)
├── TaskRunner (manages phases)
├── Device configuration
└── Connect to runner signals (via Qt::QueuedConnection)

Commission Phase:
├── Camera Thread (HighPriority)
│   └── CameraRunner
├── PLC Thread (HighPriority)
│   └── PlcRunner
└── (each device type gets a thread)

Runtime Phase (mergeToTaskThread=false):
├── Runtime Coordinator Thread
│   └── Task execution logic
├── Camera Thread
│   └── CameraRunner
├── PLC Thread
│   └── PlcRunner
└── (per-device threads retained)

Runtime Phase (mergeToTaskThread=true):
└── Runtime Thread
    ├── CameraRunner
    ├── PlcRunner
    └── (all devices merged)
```

## Related Classes

- **ITask** (`model/itask.h`) — Owns TaskRunner
- **IDeviceRunner** (`runtime/idevice_runner.h`) — Base for all device runners
- **CameraRunner** (`runtime/camera_runner.h`) — Camera I/O runner
- **PlcRunner** (`runtime/plc_runner.h`) — PLC I/O runner

