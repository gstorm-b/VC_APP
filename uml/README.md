# UML Reference

This folder contains the current UML source for the `ncr_picking` codebase.
It is intended to replace the older `uml_diagram/` folder.

The diagrams are PlantUML source files. They describe the code as it exists
now, not the desired future state. Placeholder enum values and stub classes are
marked explicitly when the implementation is not wired yet.

## Diagram Index

| File | Purpose |
|---|---|
| `01_project_overview.puml` | High-level package/module structure and dependency direction. |
| `02_device_families.puml` | Device base classes, family sub-type dispatch, concrete devices, configs, factory, and manager. |
| `03_runtime_threading.puml` | Task runner, per-device runners, QThread ownership, and runtime coordination relationships. |
| `04_localization_task.puml` | Localization task model, configuration, device assignment, signals, matcher, and pattern manager. |
| `05_matching_calibration.puml` | Matching library and calibration module relationships. |
| `06_ui_widgets.puml` | Main task UI shell, task pages, device widgets, and reusable mapping widgets. |
| `07_persistence_sequence.puml` | Project save/load sequence through repository, task factory, device factory, JSON, and image BLOBs. |
| `08_runtime_state_machines.puml` | Task runner phase, localization task state, and localization runtime cycle state transitions. |

## Current Architecture Notes

- The project is a Qt 6 / C++17 / OpenCV application built with qmake.
- `Project` owns tasks and a `DeviceManager`.
- `DeviceManager` owns device instances as `std::shared_ptr<IDevice>`.
- Device families are grouped by top-level `DeviceType`: `Camera`, `PLC`,
  `VisionOutput`, and `Robot`.
- Each device family has its own sub-type enum and abstract family base.
- `DeviceFactory` dispatches first by `DeviceType`, then by family sub-type.
- `ITask` owns a `TaskRunner`; `TaskRunner` creates one runner per assigned
  supported device.
- Runtime runner support currently exists for `Camera`, `PLC`, and
  `VisionOutput`. `Robot` devices exist as stubs but do not have a runner yet.
- `TaskLocalization` is the only concrete task currently implemented.
- Pattern data belongs to `PatternGroupManager`, while binary pattern images
  are persisted through `ProjectRepository::project_images`.
- Architecture contract tests live in
  `tests/architecture_contract_test/` and should be updated when these diagrams
  expose a new structural contract.

## Known Placeholders

- `CameraType::Realsense` and `CameraType::BaslerUSB` are enum values but
  `DeviceFactory::createCamera()` currently returns `nullptr` for them.
- `VisionOutputType::VisionSerial` is declared but has no concrete device or
  config implementation.
- `RobotType::Huayan` is declared but has no concrete implementation.
- `KawasakiRobotDevice` and `NachiRobotDevice` are minimum stubs.
- `TaskRunner` does not create a `RobotRunner`.
