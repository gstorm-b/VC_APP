# NCR Picking Source Code Documentation

Welcome to the comprehensive reference documentation for the NCR Picking project source code.

## Contents

This folder contains Markdown reference documentation for all major classes and modules in the `src/` directory.

### Key Documents

- **[INDEX.md](./INDEX.md)** — Master index and navigation guide (start here!)
- **[overview.md](./overview.md)** — Complete src/ structure and architecture patterns

### Core Classes

#### Model & Project Management
- **[Project.md](./Project.md)** — Root container for tasks, devices, and metadata
- **[ITask.md](./ITask.md)** — Abstract base for all task types
- **[TaskLocalization.md](./TaskLocalization.md)** — Image-based localization task

#### Device Framework
- **[IDevice.md](./IDevice.md)** — Abstract base for all hardware devices
- **[DeviceManager.md](./DeviceManager.md)** — Central device registry and lifecycle manager

#### Vision & Algorithms
- **[ImageMatcher.md](./ImageMatcher.md)** — Template matching engine
- **[MatchGroup.md](./MatchGroup.md)** — Pattern group container and manager

#### Runtime & Execution
- **[TaskRunner.md](./TaskRunner.md)** — Device thread manager and lifecycle coordinator
- **[LocalizationRuntimeController.md](./LocalizationRuntimeController.md)** — Localization execution orchestrator

#### Application Infrastructure
- **[AppSettings.md](./AppSettings.md)** — Persistent application preferences (singleton)

## Quick Start

1. **New to the codebase?** Start with [overview.md](./overview.md) for the big picture
2. **Looking for a specific class?** Check [INDEX.md](./INDEX.md) for the navigation table
3. **Need to understand how modules interact?** See the Architecture Patterns section in [overview.md](./overview.md)

## Documentation Format

All documentation follows a consistent structure:

- **Overview** — High-level explanation of what the class does
- **Location** — File path and namespace
- **Dependencies** — Qt modules and project files it depends on
- **Class Hierarchy** — Inheritance relationships
- **Key Concepts** — Important enumerations and nested structures
- **API Reference** — Complete listing of public methods, properties, and signals
- **Usage Example** — Real-world code snippet showing how to use the class
- **Related Classes** — Links to dependent or related classes

## Architecture Overview

```
Project
├── Tasks (ITask, TaskLocalization, etc.)
│   └── Devices (assigned to task)
│       └── Runners (CameraRunner, PlcRunner, etc.)
├── DeviceManager
│   └── All devices in project
└── Files & Persistence
    └── ProjectRepository

Execution Flow:
TaskLocalization
├── beginRuntime()
│   └── TaskRunner (manages device threads)
│       └── LocalizationRuntimeController (state machine)
│           ├── ImageMatcher (template matching)
│           ├── CameraRunner (image acquisition)
│           ├── PlcRunner (device I/O)
│           └── VisionOutputRunner (result transmission)
└── executeLocalization() (in loop)
```

## Module Descriptions

### Model (`src/model/`)
Core data structures and project management. Holds the project tree, task definitions, and serialization logic.

### Device (`src/device/`)
Hardware abstraction layer with support for cameras, PLC, robots, and vision output devices. Organized by device family.

### Runtime (`src/runtime/`)
Task execution engine. Manages device threads and coordinates device I/O with the application logic.

### Matching (`src/matching/`)
Template matching algorithms for pattern detection in images. Supports edge-based matching with sub-pixel refinement.

### Form (`src/form/`)
User interface dialogs and configuration wizards for project, task, and device setup.

### Widgets (`src/widgets/`)
Reusable UI components for image display, signal monitoring, calibration, and device control.

### Calibration (`src/calibration/`)
Camera calibration system for converting pixel coordinates to world coordinates.

### App Settings (`src/app_settings/`)
Persistent user preferences with XOR obfuscation and SHA-256 integrity checking.

### Logger (`src/logger/`)
Centralized application logging infrastructure.

### Utils (`src/utils/`)
General-purpose utilities: theme management, meta-programming helpers.

## Design Principles

1. **Separation of Concerns** — Models separate from views; device logic from communication
2. **Factory Pattern** — Decoupled object creation for devices, tasks, widgets
3. **Strategy Pattern** — Algorithm and device-type selection at runtime
4. **Qt Best Practices** — Signals/slots, parenting, meta-object system
5. **Thread Safety** — Main thread for UI; worker threads for I/O
6. **Dependency Inversion** — Code against abstractions (IDevice, ITask), not concrete classes

## Common Workflows

### Adding a New Device Type

1. Create device class inheriting from `IDevice`
2. Create device configuration class inheriting from `IDeviceCfg`
3. Create device runner inheriting from `IDeviceRunner`
4. Register with `DeviceFactory` and `DeviceWidgetFactory`
5. Create configuration UI widget

### Adding a New Task Type

1. Create task class inheriting from `ITask`
2. Implement `taskType()`, `isValid()`, `toJson()`, `fromJson()`
3. Create task configuration class
4. Register with `TaskFactory`
5. Create task configuration UI widget

## Threading Model

```
Main Thread (Qt Event Loop)
├── UI rendering and configuration
├── Project/Device/Task management
├── Signal emission (main → worker threads)
└── Signal reception (worker → main via Qt::QueuedConnection)

Worker Threads (High Priority)
├── Camera I/O (CameraRunner)
├── PLC communication (PlcRunner)
├── Robot communication (RobotRunner)
├── Vision output (VisionOutputRunner)
└── Template matching (MatchingWorker in TaskLocalization)
```

All cross-thread communication uses Qt signals/slots with `Qt::QueuedConnection`.

## File Organization

```
docs/tem_docs/
├── INDEX.md                                    (Master index)
├── README.md                                   (This file)
├── overview.md                                 (Architecture overview)
├── Project.md, ITask.md, TaskLocalization.md   (Model classes)
├── IDevice.md, DeviceManager.md                (Device framework)
├── ImageMatcher.md, MatchGroup.md              (Vision & matching)
├── TaskRunner.md                               (Runtime)
├── LocalizationRuntimeController.md            (Execution orchestrator)
└── AppSettings.md                              (Infrastructure)
```

## Related Resources

- **UML Diagrams**: See `docs/uml/` for architecture visualizations
- **Design Documents**: See `docs/` for high-level design
- **Source Code**: See `src/` for the actual implementation

## Contributing

When adding or modifying classes:

1. **Create or update Markdown documentation** following the structure above
2. **Update [INDEX.md](./INDEX.md)** if adding a new major class
3. **Update [overview.md](./overview.md)** if module structure changes
4. **Include cross-references** to related classes
5. **Add usage examples** showing real-world usage

## Documentation Generation

To convert these Markdown documents to HTML or other formats:

```bash
# Using Pandoc
pandoc overview.md -f markdown -t html -o overview.html

# Using other tools
# - Jekyll: for GitHub Pages
# - MkDocs: for simple static site
# - Sphinx: for professional documentation
```

## Questions & Feedback

For questions about specific classes or architecture:

1. Check the relevant `.md` file
2. Review related classes and usage examples
3. Consult the source code directly
4. See architecture diagrams in `docs/uml/`

---

**Project**: NCR Picking Vision System  
**Documentation Scope**: Core `src/` modules and classes  
**Format**: Markdown with reference-style structure  
**Last Updated**: 2026-06-06  
**Target Audience**: Developers adding features, fixing bugs, or onboarding to the project

