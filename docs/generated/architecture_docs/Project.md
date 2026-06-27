# Project Class Documentation

## Overview

The `Project` class is the root container for an entire NCR picking application project. It manages all tasks, devices, and project metadata in a single namespace. A project represents a complete picking application configuration including:
- Project metadata (name, author, version, timestamps)
- All tasks (localization, pick & place, etc.)
- All devices (cameras, PLC, robots, vision output)
- Task-device bindings

Every time a user creates or opens a project file, an instance of `Project` is loaded or instantiated.

## Location

- **Header**: `src/model/project.h`
- **Implementation**: `src/model/project.cpp`
- **Namespace**: `vc::model`
- **Base Class**: `QObject` (Qt Object Model with signals/slots)

## Dependencies

**Qt Modules:**
- QtCore (QObject, QVector, QJsonObject)

**Project Internal:**
- `model/itask.h` — Task interface for polymorphic task storage
- `device/idevice.h` — Device interface
- `device/device_manager.h` — Central device manager

## Class Hierarchy

```
QObject
  └── Project
```

Inherits from `QObject` to enable:
- Meta-object system (`Q_OBJECT` macro)
- Signal/slot communication
- Parenting and memory management
- Property introspection

## Q_PROPERTY Declarations

| Property | Type | READ | WRITE | NOTIFY | Description |
|----------|------|------|-------|--------|-------------|
| `name` | `QString` | `name()` | `setName()` | `nameChanged` | Project name; identifies the project |
| `version` | `QString` | `version()` | `setVersion()` | `versionChanged` | Project version string (e.g., "0.1") |
| `createdAt` | `QString` | `createdAt()` | `setCreatedAt()` | `createdAtChanged` | ISO 8601 creation timestamp |
| `updatedAt` | `QString` | `updatedAt()` | `setUpdatedAt()` | `updatedAtChanged` | ISO 8601 last modification timestamp |

All four properties are read-write and properly notify observers on change.

## Public Member Functions

### Metadata Getters & Setters

#### #### QString name() const
Returns the project name.

#### #### QString author() const
Returns the author name.

#### #### QString description() const
Returns the project description.

#### #### QString version() const
Returns the version string (default: "0.1").

#### #### QString createdAt() const
Returns the ISO 8601 creation timestamp.

#### #### QString updatedAt() const
Returns the ISO 8601 last modification timestamp.

#### #### void setName(const QString& v)
Sets the project name and emits `nameChanged()`.

#### #### void setAuthor(const QString& v)
Sets the author and emits `authorChanged()`.

#### #### void setDescription(const QString& v)
Sets the description.

#### #### void setVersion(const QString& v)
Sets the version string and emits `versionChanged()`.

#### #### void setCreatedAt(const QString& v)
Sets the creation timestamp and emits `createdAtChanged()`.

#### #### void setUpdatedAt(const QString& v)
Sets the modification timestamp and emits `updatedAtChanged()`.

### Task Management

#### #### bool addTask(ITask* task)
Adds a task to the project. Ownership is transferred to the project (stored as `std::shared_ptr<ITask>`).

**Parameters:**
- `task` — Pointer to a task instance (must not be null)

**Returns:**
- `true` if successfully added
- `false` if task already exists or is null

**Emits:** `taskCreated(QString id)` signal with the task's unique ID

#### #### bool removeTask(const QString& id)
Removes a task by its unique identifier.

**Parameters:**
- `id` — Task unique ID

**Returns:**
- `true` if task was found and removed
- `false` if task was not found

**Emits:** `taskDeleted(QString id)` signal

#### #### std::shared_ptr<vc::model::ITask> taskById(const QString& id) const
Retrieves a task by ID.

**Returns:**
- Shared pointer to the task, or `nullptr` if not found

#### #### bool changeTaskName(const QString& id, const QString& name)
Renames a task, checking for name collisions.

**Returns:**
- `true` if name changed successfully
- `false` if name is already in use or task not found

#### #### bool isTaskNameOccupied(const QString& name) const
Checks if a task name is already used in this project.

**Returns:**
- `true` if name is occupied
- `false` if name is available

#### #### const QMap<QString, std::shared_ptr<vc::model::ITask>>& getCurrentTasks() const
Returns a read-only reference to all tasks in the project.

#### #### bool moveDeviceToTask(const QString& deviceId, const QString& fromTaskId, const QString& toTaskId)
Moves a device from one task to another (re-assigns ownership).

**Parameters:**
- `deviceId` — Device unique ID
- `fromTaskId` — Source task ID
- `toTaskId` — Destination task ID

**Returns:**
- `true` if move succeeded
- `false` if source or destination task not found

### Device Management

#### #### std::shared_ptr<device::DeviceManager> deviceManager()
Returns the project-wide device manager.

**Returns:**
- Shared pointer to the `DeviceManager` instance

#### #### std::shared_ptr<vc::device::IDevice> deviceById(const QString& id)
Retrieves a device by ID.

**Returns:**
- Shared pointer to the device, or `nullptr` if not found

#### #### bool assignDeviceToTask(const QString& deviceId, const QString& taskId)
Assigns a device to a task.

**Returns:**
- `true` if assignment succeeded
- `false` if device or task not found

#### #### bool unassignDeviceFromTask(const QString& deviceId, const QString& taskId)
Unassigns a device from a task.

**Returns:**
- `true` if unassignment succeeded

### Serialization

#### #### QJsonObject toJson() const
Serializes the entire project to JSON.

**Returns:**
- A `QJsonObject` containing:
  - Metadata (name, author, description, version, timestamps)
  - All tasks (each serialized via `toJson()`)
  - Device manager state

#### #### bool fromJson(const QJsonObject& json)
Deserializes a project from JSON.

**Parameters:**
- `json` — A JSON object previously created by `toJson()`

**Returns:**
- `true` if deserialization succeeded
- `false` if JSON structure is invalid

### Validation

#### #### bool isValid() const
Checks basic project validity.

**Returns:**
- `true` if project has a non-empty name and at least one task
- `false` otherwise

## Signals

| Signal | Parameters | Emitted When |
|--------|-----------|--------------|
| `nameChanged()` | — | Project name is modified |
| `authorChanged()` | — | Author is modified |
| `versionChanged()` | — | Version is modified |
| `createdAtChanged()` | — | Created timestamp is modified |
| `updatedAtChanged()` | — | Updated timestamp is modified |
| `projectModificationOccurred()` | — | Any project data changes |
| `taskCreated(QString id)` | Task ID | A new task is added |
| `taskDeleted(QString id)` | Task ID | A task is removed |
| `taskModified(QString id)` | Task ID | A task's contents change |
| `tasksChanged()` | — | Task list or task structure changes |

## Private Members

| Member | Type | Purpose |
|--------|------|---------|
| `m_name` | `QString` | Project name |
| `m_author` | `QString` | Project author |
| `m_description` | `QString` | Project description |
| `m_version` | `QString` | Version string (default "0.1") |
| `m_createdAt` | `QString` | Creation timestamp |
| `m_updatedAt` | `QString` | Last modification timestamp |
| `m_deviceManager` | `std::shared_ptr<DeviceManager>` | Device registry and lifecycle |
| `m_tasks` | `QMap<QString, std::shared_ptr<ITask>>` | Task ID → Task mapping |
| `m_occupiedTaskNames` | `QSet<QString>` | Set of task names for uniqueness checking |

## Usage Example

```cpp
// Create a new project
auto project = std::make_shared<vc::model::Project>();
project->setName("My Picking Project");
project->setAuthor("John Doe");
project->setVersion("1.0");

// Create and add a task
auto task = new vc::model::TaskLocalization("Localize Part A");
project->addTask(task);

// Access the device manager
auto devMgr = project->deviceManager();

// Serialize to JSON
QJsonObject json = project->toJson();
```

## Threading Model

- **Main thread only**: All operations expect to be called from the Qt event loop thread
- Not thread-safe for concurrent access
- Device manager may spawn worker threads for device I/O

## Related Classes

- **ITask** (`model/itask.h`) — Interface for all task types
- **DeviceManager** (`device/device_manager.h`) — Device lifecycle and registry
- **ProjectRepository** (`model/project_repository.h`) — File I/O for projects
- **TaskFactory** (`model/task_factory.h`) — Creates task instances

