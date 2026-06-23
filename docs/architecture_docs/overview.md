# NCR Picking Project - Source Code Overview

## Project Structure

The `src/` directory contains 269 files organized across 12 major modules, plus 3 root-level header files.

---

## Root-Level Files

| File | Purpose |
|------|---------|
| `qgadget_marco.h` | Qt gadget macros and utilities |
| `setting_keys.h` | Application-wide settings keys and constants |
| `system_log_form.h / .cpp / .ui` | System logging UI form (Qt form file) |

---

## Module Breakdown

### 1. **app_settings/** (2 files)
**Purpose:** Application settings management and persistence.
- `app_settings.h / .cpp` - Central settings store

---

### 2. **calibration/** (9 files + tests)
**Purpose:** Camera and board calibration for vision system.
- `calibration_board.h / .cpp` - Calibration board model
- `calibration_board_factory.h / .cpp` - Factory for creating calibration boards
- `calibrator.h / .cpp` - Main calibration engine
- `fanuc_irvision_board.h / .cpp` - FANUC IrVision board variant
- `tests/` - Calibration tests

---

### 3. **device/** (70+ files, 5 submodules)
**Purpose:** Hardware device abstraction and management.

#### 3a. Root Device Files (11 files)
Core device abstractions and management:
- `idevice.h` - Device interface (base contract)
- `communication_device.h` - Communication protocol base
- `device_capabilities.h` - Device capability descriptors
- `device_factory.h / .cpp` - Factory for creating devices
- `device_manager.h / .cpp` - Runtime device management
- `device_registry.h / .cpp` - Device instance registry
- `device_io_interface.h` - I/O interface abstraction
- `idevice_config.h` - Device configuration interface
- `irequest.h` - Request message interface

#### 3b. **camera/** (4 files)
Camera device implementations:
- `basler_define.h` - Basler camera constants
- `camera_device.h` - Generic camera interface
- `camera_basler_gige.h / .cpp` - Basler GigE camera implementation

#### 3c. **output_device/** (11 files)
Vision output device (TCP/IP vision server):
- `vision_output_device.h` - Output device base
- `vision_output_config.h` - Output device configuration
- `vision_output_request.h` - Output request message
- `vision_tcpip_device.h / .cpp` - TCP/IP vision device server
- `vision_tcpip_device_base.h / .cpp` - TCP/IP base class
- `vision_tcpip_client_device.h / .cpp` - TCP/IP client device
- `vision_tcpip_config.h` - TCP/IP configuration
- `vision_tcpip_protocol.h` - TCP/IP protocol definitions

#### 3d. **plc/** (17 files)
PLC/Mitsubishi MC Protocol implementation:
- `plc_device.h` - PLC device interface
- `plc_request.h` - PLC request message
- `plc_value.h` - PLC value types
- `mc_protocol_device.h / .cpp` - MC protocol implementation
- `mc_context.h` - MC communication context
- `mc_context_3e.h` - MC 3E protocol variant
- `mc_context_factory.h` - Factory for MC contexts
- `mc_define.h` - MC protocol constants
- `mc_device_map.h / .cpp` - Device address mapping
- `mc_fame_3e.h / .cpp` - MC 3E protocol frame
- `mc_frame_abstract.h` - Frame base class
- `mc_msg_interface.h` - Message interface
- `mc_msg_tcp_client.h` - TCP message client
- `mc_protocol_config.h` - Protocol configuration
- `memory_utils.h` - Memory utilities for protocol

#### 3e. **robot/** (7 files)
Robot device implementations:
- `robot_device.h` - Robot device base
- `kawasaki_robot_device.h / .cpp` - Kawasaki robot implementation
- `kawasaki_robot_config.h` - Kawasaki configuration
- `nachi_robot_device.h / .cpp` - NACHI robot implementation
- `nachi_robot_config.h` - NACHI configuration

---

### 4. **form/** (35+ files, 6 submodules)
**Purpose:** UI dialogs, wizards, and device configuration forms.

#### 4a. Root Form Files (12 files)
Main dialogs and wizards:
- `add_device_wizard.h / .cpp / .ui` - Device addition wizard
- `device_widget.h` - Device widget base
- `device_widget_factory.h / .cpp` - Device widget factory
- `new_project_dialog.h / .cpp / .ui` - Project creation dialog
- `new_task_dialog.h / .cpp / .ui` - Task creation dialog
- `project_infor_setting.h / .cpp / .ui` - Project info settings
- `task_widget.h` - Task widget base

#### 4b. **camera/** (5 files)
Camera configuration UI:
- `basler_cam_select_dialog.h / .cpp / .ui` - Camera selection
- `basler_camera_widget.h / .cpp / .ui` - Camera settings widget

#### 4c. **pattern/** (10 files)
Pattern/template management UI:
- `add_pattern_image_dialog.h / .cpp / .ui` - Add pattern image
- `add_pattern_wizard.h / .cpp` - Pattern creation wizard
- `edit_pattern_wizard.h / .cpp` - Pattern editing wizard
- `pattern_canvas.h / .cpp` - Pattern drawing canvas
- `pattern_manager_dialog.h` - Pattern manager
- `pattern_setting_panel.h / .cpp` - Pattern settings
- `pattern_theme.h` - Pattern UI theme

#### 4d. **plc/** (5 files)
PLC configuration UI:
- `mitsubishi_mc_device_widget.h / .cpp / .ui` - MC device settings
- `plc_mitsu_device_wizard.h / .cpp / .ui` - PLC wizard

#### 4e. **task/** (7 files)
Task localization UI:
- `localization_dashboard_widget.h / .cpp / .ui` - Localization dashboard
- `localization_patterns_widget.h / .cpp / .ui` - Patterns panel
- `localization_setting_widget.h / .cpp / .ui` - Settings panel
- `localization_task_widget.h / .cpp / .ui` - Task widget

#### 4f. **vision_output/** (6 files)
Vision output device UI:
- `vision_tcpip_device_widget.h / .cpp / .ui` - TCP/IP device settings
- `vision_tcpip_client_device_widget.h / .cpp / .ui` - Client settings

#### 4g. **widgets/** (2 files)
Reusable form widgets:
- `status_lamp.h / .cpp` - Status indicator widget

---

### 5. **libwg/** (3 files)
**Purpose:** Reusable custom widgets library.
- `group_frame.h / .cpp` - Grouped frame widget
- `validating_line_edit.h` - Line edit with validation

---

### 6. **logger/** (2 files)
**Purpose:** Application logging infrastructure.
- `app_logger.h / .cpp` - Centralized logger

---

### 7. **matching/** (24 files)
**Purpose:** Template matching and vision algorithm implementation.
- `image_matcher.h / .cpp` - Template matching engine
- `match_pattern.h / .cpp` - Match pattern model
- `match_group.h / .cpp` - Pattern group management
- `pattern_group_manager.h / .cpp` - Group manager
- `match_config_property_adapter.h / .cpp` - Config binding
- `match_pattern_config.h / .cpp` - Pattern configuration
- `match_pattern_layer.h` - Pattern layer model
- `match_params.h / .cpp` - Match algorithm parameters
- `match_box_gripper.h` - Bounding box utility
- `match_object.h` - Match result object
- `edge_match_config.h` - Edge matching configuration
- `imatch_type_config.h / .cpp` - Match type interface
- `matching_types.h` - Match type definitions
- `manager_result.h` - Match manager result
- `vision_utils.h / .cpp` - Vision algorithm utilities
- `utils_block_max.h / .cpp` - Block max filter

---

### 8. **model/** (18 files)
**Purpose:** Core data models and task/project management.
- `project.h / .cpp` - Project model
- `project_repository.h / .cpp` - Project persistence
- `itask.h / .cpp` - Task interface base
- `task_define.h` - Task type definitions
- `task_factory.h / .cpp` - Task creation factory
- `task_localization.h / .cpp` - Localization task model
- `task_localization_config.h` - Localization configuration
- `task_state_machine.h` - Task state machine
- `pick_and_place_task.h` - Pick & place task model
- `localization_pipeline.h / .cpp` - Localization execution pipeline
- `localization_runtime_controller.h / .cpp` - Runtime control
- `localization_signal_mapper.h / .cpp` - Signal mapping
- `localization_recovery_policy.h` - Error recovery policy
- `localization_fault_code.h` - Fault code definitions
- `task_device_binding.h` - Device binding contract
- `camera_map_entry.h` - Camera mapping model
- `isignal_group.h` - Signal group interface
- `itask_config.h` - Task configuration interface

---

### 9. **runtime/** (9 files)
**Purpose:** Task execution runtime and device runners.
- `task_runner.h / .cpp` - Task execution engine
- `device_runner.h` - Device command execution base
- `idevice_runner.h` - Device runner interface
- `camera_runner.h` - Camera execution runner
- `plc_runner.h` - PLC command runner
- `vision_output_runner.h` - Vision output runner
- `device_command.h` - Device command definition
- `device_command_queue.h` - Command queue

---

### 10. **utils/** (3 files)
**Purpose:** General utility functions and helpers.
- `meta_utils.h` - Meta-programming utilities
- `theme_manager.h / .cpp` - Application theme management

---

### 11. **widgets/** (30+ files, 3 submodules)
**Purpose:** Reusable UI components and interactive widgets.

#### 11a. Root Widgets (15 files)
Core widgets:
- `pattern_tree_widget.h / .cpp` - Pattern hierarchy tree
- `project_tree_widget.h / .cpp` - Project hierarchy tree
- `camera_mapping_widget.h / .cpp` - Camera-to-task mapping UI
- `signals_map_widget.h / .cpp` - Signal mapping UI
- `signals_monitor_widget.h / .cpp` - Signal monitoring
- `task_event_log_widget.h / .cpp` - Task event log
- `clamp.h / .cpp` - Clamp utility widget
- `group_frame_widget.h / .cpp` - Grouped frame widget
- `lamp_button.h` - On/off indicator button
- `no_wheel_combobox.h / .cpp` - Combobox without scroll wheel
- `no_wheel_double_spinbox.h` - Spinbox without scroll wheel
- `no_wheel_spinbox.h` - Spinbox without scroll wheel

#### 11b. **calibration/** (6 files)
Calibration UI components:
- `calibration_board_dialog.h / .cpp` - Calibration board dialog
- `calibration_points_table.h / .cpp` - Points table display
- `calibration_threshold_dialog.h / .cpp` - Threshold settings

#### 11c. **image_widget/** (12 files)
Image display and interaction:
- `image_widget.h / .cpp` - Interactive image display
- `image_view_only.h / .cpp` - Read-only image viewer
- `item_pixmap_bounding.h` - Bounding box item
- `item_roi.h / .cpp` - ROI rectangle item
- `item_roi_rotated.h` - Rotated ROI item
- `item_gripper_box.h` - Gripper bounding box item
- `item_picking_pos.h / .cpp` - Picking position marker

#### 11d. **plc_widget/** (8+ files)
PLC monitoring and control UI:
- `devices_monitor_widget.h / .cpp` - Device monitoring
- `d_devices_table.h / .cpp` - Device table display
- `device_row_delegate.h / .cpp` - Device row rendering
- (Additional PLC widget files)

---

## Module Dependencies Summary

```
app_settings
    ↓
model (Project, Task, Localization)
    ↓
device (Cameras, PLC, Robots, Vision Output)
    ↓
matching (Template Matching)
    ↓
runtime (Task Execution)
    ↓
form (UI Dialogs)
    ↓
widgets (UI Components)
    ↓
logger & utils (Supporting)
```

---

## Key Architectural Patterns

1. **Factory Pattern**: Device factories, task factories, widget factories
2. **Strategy Pattern**: Device runners, match type configs
3. **Model-View Pattern**: Task models with corresponding UI widgets
4. **Observer Pattern**: Qt signals/slots throughout
5. **Component-based**: Modular device implementations (camera, PLC, robot, vision output)

---

## Build System

- **Build Tool**: Qt (qmake or CMake)
- **Qt Modules**: Core, Gui, Widgets, Network (inferred)
- **Compiler Target**: MSVC 2022, 64-bit (Windows)

---

## Documentation Index

For detailed documentation of specific classes and modules, see:
- [Class Documentation](./classes/) - Detailed API for individual classes
- [Module Guides](./modules/) - Module-specific documentation
- Architecture diagrams: See `docs/uml/` for PlantUML diagrams

