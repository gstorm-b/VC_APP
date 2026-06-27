# NCR Picking Source Code Documentation Index

Welcome to the comprehensive source code documentation for the NCR Picking project. This folder contains reference documentation for all major classes and modules.

---

## Quick Navigation

### Architecture Overview
- [**overview.md**](./overview.md) — Complete src/ structure, module breakdown, and architecture patterns

### Core Model Classes

| Class | File | Purpose |
|-------|------|---------|
| `Project` | [Project.md](./Project.md) | Root container for tasks, devices, and metadata |
| `ITask` | [ITask.md](./ITask.md) | Abstract base for all task types (Localization, Pick & Place, etc.) |
| `TaskLocalization` | [TaskLocalization.md](./TaskLocalization.md) | Image-based localization task implementation |

### Device Framework

| Class | File | Purpose |
|-------|------|---------|
| `IDevice` | [IDevice.md](./IDevice.md) | Abstract base for all hardware devices |
| `DeviceManager` | [DeviceManager.md](./DeviceManager.md) | Central registry and lifecycle manager for devices |

### Vision & Matching

| Class | File | Purpose |
|-------|------|---------|
| `ImageMatcher` | [ImageMatcher.md](./ImageMatcher.md) | Edge-based template matching engine |
| `MatchGroup` | [MatchGroup.md](./MatchGroup.md) | Container for multiple template patterns with shared config |

### Runtime & Execution

| Class | File | Purpose |
|-------|------|---------|
| `TaskRunner` | [TaskRunner.md](./TaskRunner.md) | Device thread manager and phase coordinator |
| `LocalizationRuntimeController` | [LocalizationRuntimeController.md](./LocalizationRuntimeController.md) | Localization execution state machine and orchestrator |

### Application Infrastructure

| Class | File | Purpose |
|-------|------|---------|
| `AppSettings` | [AppSettings.md](./AppSettings.md) | Singleton for persistent application preferences |

---

## Module Organization

The `src/` directory is organized into 12 core modules:

### 1. **Model** (`src/model/`)
Core data structures for projects, tasks, and configurations.
- **Main classes**: Project, ITask, TaskLocalization, TaskFactory
- **Key patterns**: Factory, Strategy (task types)

### 2. **Device** (`src/device/`)
Hardware device abstraction and management.
- **Main classes**: IDevice, DeviceManager, DeviceFactory
- **Submodules**: 
  - `camera/` — Camera implementations (Basler GigE)
  - `plc/` — Mitsubishi MC Protocol
  - `robot/` — Kawasaki & NACHI robots
  - `output_device/` — Vision output servers
- **Key patterns**: Factory, Strategy (device types)

### 3. **Runtime** (`src/runtime/`)
Task execution engine and device runners.
- **Main classes**: TaskRunner, DeviceRunner, CameraRunner, PlcRunner
- **Key patterns**: Command pattern (device commands), Strategy (device runners)

### 4. **Matching** (`src/matching/`)
Template matching and vision algorithms.
- **Main classes**: ImageMatcher, PatternGroupManager, MatchGroup
- **Algorithms**: Edge-based matching, sub-pixel refinement, overlap filtering

### 5. **Form** (`src/form/`)
UI dialogs, wizards, and device configuration forms.
- **Main components**: Add device wizard, new project/task dialogs
- **Submodules**: `camera/`, `pattern/`, `plc/`, `task/`, `vision_output/`, `widgets/`

### 6. **Widgets** (`src/widgets/`)
Reusable UI components for monitoring and visualization.
- **Components**: Image display, pattern tree, signal monitoring, calibration widgets
- **Submodules**: `image_widget/`, `calibration/`, `plc_widget/`

### 7. **Calibration** (`src/calibration/`)
Camera and board calibration for vision system.
- **Main classes**: Calibrator, CalibrationBoard, CalibrationBoardFactory

### 8. **App Settings** (`src/app_settings/`)
Application preferences and configuration.
- **Main class**: AppSettings (singleton)

### 9. **Logger** (`src/logger/`)
Centralized application logging.
- **Main class**: AppLogger

### 10. **Utils** (`src/utils/`)
General utility functions and helpers.
- **Utilities**: Theme manager, meta-programming utilities

### 11. **Library Widgets** (`src/libwg/`)
Reusable custom Qt widgets.
- **Widgets**: GroupFrame, ValidatingLineEdit

---

## Architecture Patterns

### 1. **Factory Pattern**
- `DeviceFactory` — Creates device instances
- `TaskFactory` — Creates task instances
- `DeviceWidgetFactory` — Creates configuration UI widgets
- Benefits: Decouples creation from usage

### 2. **Strategy Pattern**
- `DeviceRunner` implementations — Different runners per device type
- `MatchTypeConfig` implementations — Different matching algorithms
- Benefits: Algorithm selection at runtime

### 3. **Model-View Pattern**
- Task models (`ITask`, `TaskLocalization`) with corresponding UI widgets
- Benefits: Separation of concerns, independent evolution

### 4. **Observer Pattern (Qt Signals/Slots)**
- Extensive use throughout for event-driven communication
- Property changes trigger signals
- Benefits: Loose coupling, real-time updates

### 5. **Component-Based Architecture**
- Modular device implementations (camera, PLC, robot, vision output)
- Each component has a factory, runner, and UI widget
- Benefits: Easy to add new device types

### 6. **Singleton Pattern**
- `AppSettings` — Global application preferences
- Benefits: Single source of truth for configuration

---

## Development Workflows

### Adding a New Device Type

1. **Create device class** (`src/device/mydevice/my_device.h`)
   - Inherit from `IDevice`
   - Implement pure virtual methods

2. **Create device configuration** (`src/device/mydevice/my_device_config.h`)
   - Inherit from `IDeviceCfg`

3. **Create device runner** (`src/runtime/my_device_runner.h`)
   - Inherit from `IDeviceRunner`
   - Implement command execution

4. **Register with factories**
   - Update `DeviceFactory` to create instances
   - Update `DeviceWidgetFactory` for UI

5. **Create configuration UI** (`src/form/mydevice/my_device_widget.h`)
   - Allow users to configure device

### Adding a New Task Type

1. **Create task class** (`src/model/my_task.h`)
   - Inherit from `ITask`
   - Implement pure virtual methods

2. **Create task configuration** (`src/model/my_task_config.h`)
   - Configuration parameters

3. **Register with factory**
   - Update `TaskFactory` to create instances

4. **Create task widget** (`src/form/task/my_task_widget.h`)
   - Allow users to interact with task

---

## Key Design Principles

1. **Separation of Concerns**
   - Models separate from views/UI
   - Device logic separate from communication

2. **Single Responsibility**
   - Each class has one reason to change
   - Example: DeviceManager manages lifecycle, not communication

3. **Dependency Inversion**
   - Depend on abstractions (`IDevice`, `ITask`), not concrete classes
   - Enables easy testing and extension

4. **Qt Best Practices**
   - Use `Q_OBJECT`, `Q_PROPERTY`, signals/slots
   - Parent-child ownership for memory management
   - Thread-safe communication via queued connections

5. **Thread Safety**
   - All device runners operate in worker threads
   - Main thread used for UI and configuration
   - Signal/slot connections use `Qt::QueuedConnection` for thread safety

---

## Threading Model

```
Main Thread (Qt Event Loop)
├── UI & Configuration
├── Signal/slot connections (emit)
└── Project/Device management

Worker Threads
├── Camera I/O (CameraRunner)
├── PLC communication (PlcRunner)
├── Robot communication (RobotRunner)
├── Vision output (VisionOutputRunner)
└── Template matching (MatchingWorker)
```

---

## Building the Documentation

To generate HTML documentation from these markdown files:

```bash
# Using Pandoc
pandoc overview.md -f markdown -t html -o overview.html

# Using other tools: Jekyll, MkDocs, Sphinx, etc.
```

---

## Related Resources

- **Architecture Diagrams**: See `docs/uml/` for PlantUML diagrams
- **Project Structure**: See `docs/` for high-level design documents
- **API Reference**: This folder (tem_docs/)

---

## Documentation Format

All documentation follows these conventions:

- **Markdown format** for easy reading and version control
- **Consistent structure**: Overview → Dependencies → API → Examples
- **No code fences** except in Usage Examples section
- **Inline code** for method signatures
- **Tables** for properties, enumerations, and signals
- **Cross-references** to related classes

---

## Contributing

When adding new classes or making significant changes:

1. Update **overview.md** if module structure changes
2. Create a new markdown file for the class (ClassName.md)
3. Follow the documentation structure above
4. Update this INDEX.md with links

---

**Last Updated**: 2026-06-06
**Project**: NCR Picking Vision System
**Documentation Scope**: src/ modules and core classes

