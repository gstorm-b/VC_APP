# DeviceManager Class Documentation

## Overview

`DeviceManager` is the centralized registry and lifecycle manager for all hardware devices in an NCR picking project.

Responsibilities:
- Register and unregister devices
- Allocate unique device IDs
- Enforce device-type limits (max cameras, PLCs, etc.)
- Provide device lookup by ID or name
- Manage device name uniqueness
- Serialize/deserialize device state

A `DeviceManager` instance is created by each `Project` and manages all devices for that project.

## Location

- **Header**: `src/device/device_manager.h`
- **Implementation**: `src/device/device_manager.cpp`
- **Namespace**: `vc::device`
- **Base Class**: `QObject`

## Dependencies

**Qt Modules:**
- QtCore (QObject, QJsonObject, QMutex, QString)

**Project Internal:**
- `device/idevice.h` — Device interface
- `model/project.h` — Parent project

## Constructor & Destructor

#### #### DeviceManager(vc::model::Project* proj = nullptr, QObject* parent = nullptr)
Creates a device manager for a project.

**Parameters:**
- `proj` — Parent project (can be nullptr)
- `parent` — Qt parent object

#### #### ~DeviceManager()
Destructor; releases all devices.

## Public Methods

### Project Reference

#### #### vc::model::Project* project()
Returns a pointer to the parent project.

### Device Limits

#### #### bool isLimitReached(DeviceType type) const
Checks if the maximum number of devices of a given type has been reached.

**Parameters:**
- `type` — Device type (Camera, PLC, Robot, VisionOutput)

**Returns:**
- `true` if limit reached
- `false` otherwise

### Device Registration Workflow

The typical device creation flow uses pending IDs:

```cpp
// 1. Allocate a pending ID (before device is fully constructed)
QString tempId = deviceManager->allocatePendingId();

// 2. Construct device with temp ID
auto device = new CameraBaslerGige(tempId, "My Camera");

// 3. Reserve the device (preparation phase)
deviceManager->reserveDevice(tempId, "My Camera", device_shared_ptr);

// ... device initialization, configuration ...

// 4. Commit the device (finalize registration)
deviceManager->commitDevice(tempId, "My Camera", device_shared_ptr);
```

#### #### QString allocatePendingId()
Allocates a temporary device ID for a device under construction.

**Returns:**
- A unique pending ID string

**Note:** The pending ID is held until `commitDevice()` or `releasePendingId()` is called.

#### #### bool reserveDevice(const QString& id, const QString& name, std::shared_ptr<IDevice> device)
Reserves a device (preparation phase) before final commitment.

**Parameters:**
- `id` — Pending device ID (from `allocatePendingId()`)
- `name` — Device name
- `device` — Shared pointer to the device instance

**Returns:**
- `true` if reservation succeeded
- `false` if name conflict or type limit reached

**Emits:** `deviceCreated(id)` signal

#### #### bool commitDevice(const QString& id, const QString& name, std::shared_ptr<IDevice> device)
Commits a device to the registry (finalization phase).

**Parameters:**
- `id` — Device ID (previously reserved)
- `name` — Device name
- `device` — Shared pointer to the device instance

**Returns:**
- `true` if commit succeeded
- `false` if device not found or name conflict

#### #### void releaseDevice(const QString& id)
Removes a device from the registry (final cleanup).

**Parameters:**
- `id` — Device ID to release

**Emits:** `deviceDeleted(id)` signal

#### #### void releasePendingId(const QString& id)
Cancels a pending ID allocation (device creation failed or aborted).

**Parameters:**
- `id` — Pending ID to release

### Device Lookup

#### #### std::shared_ptr<IDevice> deviceById(const QString& id)
Retrieves a device by its unique ID.

**Parameters:**
- `id` — Device ID

**Returns:**
- Shared pointer to the device, or `nullptr` if not found

#### #### QString getDeviceName(const QString& id)
Gets the name of a device by ID.

**Parameters:**
- `id` — Device ID

**Returns:**
- Device name, or empty string if device not found

#### #### bool isNameExists(const QString& name)
Checks if a device name is already in use.

**Parameters:**
- `name` — Name to check

**Returns:**
- `true` if name exists
- `false` if available

#### #### bool isOccupied(const QString& id)
Checks if a device ID is occupied (registered).

**Parameters:**
- `id` — Device ID

**Returns:**
- `true` if device exists
- `false` otherwise

### Device Name Management

#### #### bool changeDeviceName(const QString& id, const QString newName)
Renames a device, checking for name collisions.

**Parameters:**
- `id` — Device ID
- `newName` — New device name

**Returns:**
- `true` if rename succeeded
- `false` if new name is already in use

**Emits:** `deviceModified(id)` signal

### Device Filtering by Type

#### #### QMap<QString, QString> commDevices()
Returns all PLC/communication devices as a map: device ID → device name.

#### #### QMap<QString, QString> outputDevices()
Returns all vision output devices: device ID → device name.

#### #### QMap<QString, QString> cameraDevices()
Returns all camera devices: device ID → device name.

#### #### QStringList commDevicesNameList()
Returns names of all PLC devices.

#### #### QStringList outputDevicesNameList()
Returns names of all vision output devices.

#### #### QStringList cameraDevicesNameList()
Returns names of all camera devices.

### Device Type Information

#### #### QStringList& getDeviceTypeList()
Returns the list of all available device type names.

#### #### QStringList& getSubDeviceTypeList(DeviceType type)
Returns the list of sub-types for a given device type.

**Example:** For `DeviceType::PLC`, returns ["Mitsubishi MC", "Kawasaki", ...].

### Device Collection

#### #### const QMap<QString, std::shared_ptr<IDevice>>& getCurrentDevices()
Returns a read-only reference to all registered devices.

**Returns:**
- Map of device ID → device shared pointer

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `devicesChanged()` | — | Any device is added, removed, or modified |
| `deviceCreated(QString id)` | Device ID | A device is successfully registered |
| `deviceDeleted(QString id)` | Device ID | A device is removed |
| `deviceModified(QString id)` | Device ID | A device's properties change |

## Private Methods

#### #### QString generateId()
Generates a unique device ID (internal use).

#### #### void deviceNameCheck(IDevice* device)
Validates that a device's name is unique (internal use).

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `deviceInstances` | `QMap<QString, std::shared_ptr<IDevice>>` | All registered devices |
| `dvTypeStrings` | `QStringList` | Device type names |
| `subDeviceTypeLists` | `QMap<DeviceType, QStringList>` | Sub-type names per type |
| `occupiedNames` | `QSet<QString>` | Registered device names |
| `maxLimits` | `QMap<DeviceType, int>` | Device type limits |
| `currentCounts` | `QMap<DeviceType, int>` | Current device counts per type |
| `currentMaxId` | `int` | Counter for ID generation |
| `freedIds` | `std::set<int>` | Reusable ID indices |
| `mutex` | `QMutex` | Thread synchronization |
| `parent_proj` | `vc::model::Project*` | Parent project |

## Usage Example

```cpp
// Create a device manager (normally done by Project)
auto devMgr = std::make_shared<vc::device::DeviceManager>(project);

// Allocate a pending ID
QString tempId = devMgr->allocatePendingId();

// Create a camera device
auto camera = std::make_shared<vc::device::CameraBaslerGige>(
    tempId, 
    "Front Camera"
);

// Reserve the device
if (!devMgr->reserveDevice(tempId, "Front Camera", camera)) {
    qWarning() << "Failed to reserve device";
    return;
}

// Configure and initialize camera
camera->setDeviceConfig(config);
camera->deviceConnect();

// Commit the device
if (!devMgr->commitDevice(tempId, "Front Camera", camera)) {
    qWarning() << "Failed to commit device";
    return;
}

// Retrieve device later
auto retrievedDevice = devMgr->deviceById(tempId);
if (retrievedDevice) {
    qDebug() << "Device found:" << retrievedDevice->name();
}

// List all cameras
auto cameras = devMgr->cameraDevices();
for (auto it = cameras.begin(); it != cameras.end(); ++it) {
    qDebug() << "Camera ID:" << it.key() << "Name:" << it.value();
}

// Release device when done
devMgr->releaseDevice(tempId);
```

## Threading Model

- **Main thread only**: All operations are not thread-safe by design
- Device manager operates on the main Qt event loop
- Mutex is used internally for protection against concurrent access attempts

## Related Classes

- **Project** (`model/project.h`) — Creates and owns device manager
- **IDevice** (`device/idevice.h`) — Base device interface
- **DeviceFactory** (`device/device_factory.h`) — Creates device instances

