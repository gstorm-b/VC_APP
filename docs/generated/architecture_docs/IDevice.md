# IDevice Interface Documentation

## Overview

`IDevice` is the abstract base class for all hardware devices in the NCR picking system. It defines the contract for device connection, communication, configuration, and status monitoring.

Concrete implementations include:
- **Cameras** (Basler GigE)
- **PLC devices** (Mitsubishi MC Protocol)
- **Robots** (Kawasaki, NACHI)
- **Vision output devices** (TCP/IP vision server)

Every physical device connected to the system is represented as an `IDevice` subclass instance.

## Location

- **Header**: `src/device/idevice.h`
- **Implementation**: Various files in `src/device/`
- **Namespace**: `vc::device`
- **Base Class**: `QObject` (Qt Object Model with signals/slots)

## Dependencies

**Qt Modules:**
- QtCore (QObject, QMutex, QString, QJsonObject)

**Project Internal:**
- `irequest.h` — Request message interface
- `idevice_config.h` — Device configuration interface
- `logger/app_logger.h` — Logging

## Class Hierarchy

```
QObject
  └── IDevice (abstract)
       ├── CameraDevice
       │    └── CameraBaslerGige (concrete)
       ├── PlcDevice
       │    └── McProtocolDevice (concrete)
       ├── RobotDevice
       │    ├── KawasakiRobotDevice (concrete)
       │    └── NachiRobotDevice (concrete)
       └── VisionOutputDevice
            ├── VisionTcpipDevice (server)
            └── VisionTcpipClientDevice (client)
```

## Enumerations

### ConnectStatus

Represents the device connection state.

| Value | Description |
|-------|-------------|
| `NoConnection` | Device has never been connected |
| `Disconnected` | Device was connected, now disconnected |
| `Connected` | Device is fully connected and operational |
| `LostConnected` | Device lost connection unexpectedly |
| `ConnectFailed` | Connection attempt failed |
| `Connecting` | Connection in progress; transport active but links not all up yet |

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `name` | `QString` | `name()` | `setName()` | `nameChanged` | Human-readable device name |
| `id` | `QString` | `id()` | — | — | Unique device ID (immutable) |

## Pure Virtual Methods (Must Override)

#### #### bool deviceConnect() = 0
Initiates connection to the device.

**Returns:**
- `true` if connection started successfully
- `false` if connection failed

**Side Effects:**
- Updates `m_connect_status` via `setConnectionStatus()`
- Emits `connectStatusChanged()` signal

#### #### bool deviceDisconnect() = 0
Terminates connection to the device.

**Returns:**
- `true` if disconnection succeeded
- `false` otherwise

#### #### bool isDeviceConnected() const = 0
Checks if the device is currently connected and operational.

**Returns:**
- `true` if device is connected and ready
- `false` otherwise

#### #### DeviceType deviceType() const = 0
Returns the concrete type of this device.

**Returns:**
- One of: `Camera`, `PLC`, `Robot`, `VisionOutput`, etc.

#### #### bool pushRequest(IRequest* request) = 0
Enqueues a request for execution on this device.

**Parameters:**
- `request` — A command or query to send to the device

**Returns:**
- `true` if request was successfully queued
- `false` if queue is full or device is not connected

## Public Getters & Setters

### Identification

#### #### QString id() const
Returns the unique device identifier. Assigned at construction, immutable.

#### #### QString name() const
Returns the human-readable device name.

#### #### void setName(const QString& v)
Sets the device name and emits `nameChanged()` if the value changes.

### Connection Management

#### #### ConnectStatus connectStatus() const
Returns the current connection state of the device.

#### #### void setConnectionStatus(ConnectStatus status, QString msg = "")
Updates the device connection status and emits appropriate signals.

**Parameters:**
- `status` — New connection state
- `msg` — Optional error message (used when `status == ConnectFailed`)

**Side Effects:**
- Emits `connectStatusChanged(status)` signal
- If status is `ConnectFailed`, emits `connectionFailed(msg)` signal

#### #### QString lastMsg() const
Returns the last error or status message from the device.

### Task Assignment

A device can be assigned to a task (e.g., a camera assigned to a localization task).

#### #### void setAssignedTaskId(const QString& taskId)
Assigns the device to a task by task ID.

#### #### QString assignedTaskId() const
Returns the ID of the task this device is assigned to (or empty string if unassigned).

### Configuration

#### #### void setDeviceConfig(IDeviceCfg* cfg)
Sets the device configuration.

**Parameters:**
- `cfg` — A device-specific configuration object

**Emits:** `configChanged()`

#### #### IDeviceCfg* deviceConfig()
Returns a clone of the device's current configuration.

**Returns:**
- A newly allocated copy of the configuration (caller must delete)

### Manager Reference

#### #### void setManager(DeviceManager* manager)
Links this device to its manager (usually called by `DeviceManager` during registration).

#### #### DeviceManager* deviceManager()
Returns a pointer to the manager that owns this device.

## Serialization

#### #### QJsonObject toJson() const
Serializes the device to JSON, including:
- Device ID and name
- Connection status
- Device-specific configuration

**Returns:**
- A `QJsonObject` containing device metadata

#### #### bool fromJson(const QJsonObject& obj)
Deserializes device state from JSON.

**Parameters:**
- `obj` — JSON object previously created by `toJson()`

**Returns:**
- `true` if deserialization succeeded

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `nameChanged()` | — | Device name is modified |
| `connectStatusChanged(ConnectStatus)` | New status | Device connection state changes |
| `connectionFailed(QString msg)` | Error message | Connection attempt fails |
| `configChanged()` | — | Device configuration is updated |
| `requestProcessed(IRequest*)` | Request pointer | A request completes or fails |

## Constructor

#### #### IDevice(QString id, QString name, QObject* parent = nullptr)
Constructs a device with a unique ID and name.

**Parameters:**
- `id` — Unique device identifier
- `name` — Human-readable device name
- `parent` — Qt parent object

**Initialization:**
- `m_connect_status` is set to `NoConnection`
- `m_abstract_cfg` is set to `nullptr`
- `m_manager` is set to `nullptr`

## Destructor

#### #### virtual ~IDevice() = default
Virtual destructor for proper cleanup in subclasses.

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `m_id` | `QString` | Unique device identifier |
| `m_name` | `QString` | Human-readable name |
| `m_connect_status` | `ConnectStatus` | Current connection state |
| `m_assignedTaskId` | `QString` | Task this device is bound to |
| `m_last_msg` | `QString` | Last error/status message |
| `m_abstract_cfg` | `IDeviceCfg*` | Device configuration |
| `m_manager` | `DeviceManager*` | Owning manager |

## Usage Example

```cpp
// Create a concrete device (Basler camera)
auto camera = new vc::device::CameraBaslerGige("cam_001", "Front Camera");

// Create configuration
auto cfg = new vc::device::BaslerCameraConfig();
cfg->setIpAddress("192.168.1.100");
cfg->setFrameRate(30);
camera->setDeviceConfig(cfg);

// Assign to a task
camera->setAssignedTaskId("task_localize_001");

// Connect
if (camera->deviceConnect()) {
    qDebug() << "Camera connected";
    qDebug() << "Status:" << (int)camera->connectStatus();
    
    // Create and send a request
    auto req = new CameraGrabRequest();
    camera->pushRequest(req);
} else {
    qDebug() << "Failed to connect:" << camera->lastMsg();
}
```

## Threading Model

- **Main thread**: All property getters/setters, connection status queries
- **Worker threads**: Device communication (camera I/O, PLC requests, etc.)
- **Thread-safe**: `QMutex` may be used internally by implementations

## Concrete Implementations

### CameraBaslerGige
Basler GigE camera via Basler Pylon SDK.

### McProtocolDevice
Mitsubishi Melsec MC Protocol via TCP/IP.

### KawasakiRobotDevice / NachiRobotDevice
Kawasaki and NACHI robots via serial/TCP.

### VisionTcpipDevice / VisionTcpipClientDevice
Vision output server and client for sending results to external systems.

## Related Classes

- **IRequest** (`device/irequest.h`) — Base for device requests/commands
- **IDeviceConfig** (`device/idevice_config.h`) — Base for device configuration
- **DeviceManager** (`device/device_manager.h`) — Device lifecycle and registry
- **DeviceFactory** (`device/device_factory.h`) — Creates device instances

