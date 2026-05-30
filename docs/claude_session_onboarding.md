# Claude Session Onboarding

> **Audience.** Claude (or any AI coding assistant) starting a fresh session on
> the `ncr_picking` project. Read this first to acquire the minimum context
> needed before touching code.

## TL;DR — read these, in order

1. **[design_rules.md](design_rules.md)** — non-negotiable. Coding conventions,
   ownership rules, signal/slot patterns, device-family conventions, themed-QSS
   rules, doc language rule. **Always skim §11 (Process) and §16 (Language)
   before writing code.** Other sections are read-on-demand.
2. **[later_todo_list.md](later_todo_list.md)** — deferred items. Skim the
   index so you know what is *already known to be broken / incomplete* and
   don't re-flag it as a new finding. If your task touches an item here, link
   to it; if you finish one, remove it.
3. **[device_type.md](device_type.md)** — current DeviceType taxonomy
   (Camera / PLC / VisionOutput / Robot) plus their sub-types. Anything that
   adds a device, a sub-type, or routes by `DeviceType` must match this.

That is the floor. Stop here if the task is small and local (e.g., a single
widget tweak, a typo fix). For anything larger, also read the
**topic-specific** doc(s) below.

---

## Topic-specific reading

Pick the section that matches the area you are touching. Do **not** read all of
them.

### Working on a task widget / task config

- **[task_localize_setting_widget.md](task_localize_setting_widget.md)** —
  reference spec for the localization setting widget. Mirrors the pattern any
  future task-setting widget should follow (device combos, mapping widgets,
  push-config-on-every-change).
- **[signal_map/signals_map_widget.md](signal_map/signals_map_widget.md)** —
  generic mapping editor used by task settings to bind logical signals to
  PLC tags. Read before extending it or building a sibling widget.
- **[signal_map/signals_monitor_widget.md](signal_map/signals_monitor_widget.md)**
  — design notes for the (planned) monitor widget that visualises live values
  of mapped signals. Read if the task is "build the monitor widget".

### Working on calibration

- **[calibration_module.md](calibration_module.md)** — full reference for the
  C++ calibration submodule (`calib_board_recognize`): board abstractions,
  homography + plane fit, image↔robot conversion. Read before touching
  anything under `calib_board_recognize/` or `src/widgets/calibration/`.
- **[calibration_tables_point.md](calibration_tables_point.md)** — UI spec for
  the calibration points table (6-column editor, signals). Read before
  touching `src/widgets/calibration/calibration_points_table.*`.

### Working on a device family (PLC / Camera / VisionOutput / Robot)

- **[design_rules.md](design_rules.md) §12 Device Family Modules** — the
  abstract-base + concrete sub-type pattern is the load-bearing convention.
  Every device family currently follows it; new ones must too.
- **[device_type.md](device_type.md)** — confirm the sub-type you are adding
  is on the target taxonomy.
- **[later_todo_list.md](later_todo_list.md)** — items #15–#20 capture
  refactor leftovers in PLC/VisionOutput/Robot. Check these before flagging
  new ones in the same area.

### Adding a TCP/IP-based device

- **[design_rules.md](design_rules.md) §13 TCP / Network Patterns** — required
  pattern for client/server objects, threading, lifecycle.

### Adding a themed widget (UI)

- **[design_rules.md](design_rules.md) §15 Themed Form Stylesheets** —
  required. No inline `setStyleSheet`, no hard-coded colors; everything goes
  via `resrc/styles/<widget>_{dark,light}.qss` + `ThemeManager`.

---

## Mandatory operating rules (always on)

These are in `design_rules.md` but worth surfacing here because they shape
every interaction:

- **Rule 11.1 — Confirm before implementing.** When the user describes a task,
  restate scope and design choices and **ask for confirmation** before writing
  code. This is the most-repeated user instruction in this project; violating
  it wastes a turn.
- **Rule 11.2 — Flag, don't silently fix.** If you notice an unrelated bug or
  smell while doing a task, **flag it to the user and/or add it to
  `later_todo_list.md`** — do not silently fix it in the same change.
- **Rule 16 — Code is English, chat can be Vietnamese.** All identifiers,
  comments, log messages, commit-style notes inside code/docs are English.
  User-facing conversation may be Vietnamese (the user often writes in
  Vietnamese); reply in the same language they used. Docs (`docs/*.md`) are
  English.
- **No legacy/migration shims.** The project has not been deployed to a
  customer yet. When refactoring, change the code directly — do not add
  backwards-compatibility wrappers, `_deprecated` aliases, or migration code
  unless the user explicitly asks.

---

## Project-shape cheat sheet

Enough orientation to navigate without reading every header:

- **Build.** qmake project, `ncr_picking.pro`. Qt 6, C++17, OpenCV 4
  (`opencv_world`). The user builds locally; you do **not** run the build
  unless asked.
- **Top-level layout.**
  - `src/device/` — device families (`camera/`, `plc/`, `output_device/`,
    `robot/`). Each family has an abstract header-only base + concrete
    `.h/.cpp` siblings per sub-type.
  - `src/model/` — `ITask`, concrete tasks (`TaskLocalization`), `Project`,
    `DeviceManager`.
  - `src/runtime/` — per-device runners that live on the task runtime thread
    (`CameraRunner`, `PlcRunner`, `VisionOutputRunner`).
  - `src/form/` — top-level widgets (one per task / per device). Each pairs
    with a `.ui` file.
  - `src/widgets/` — reusable widget building blocks (`signals_map_widget`,
    `camera_mapping_widget`, `property_browser/`, `calibration/`).
  - `resrc/styles/` — `<widget>_{dark,light}.qss` files loaded by
    `ThemeManager`.
- **Persistence.** JSON via `toJson()/fromJson()` on every config and device.
  Image BLOBs travel through `ProjectRepository::project_images` table (see
  `TaskLocalization::getTaskImageMap()`).
- **Device sub-type dispatch.** Each family declares a `<Family>Type` enum
  and a `DEVICE_JSK_<FAMILY>_TYPE` JSON key. `DeviceFactory::create<Family>Device()`
  switches on the enum.

---

## What NOT to do on a fresh session

- Do not read `docs/old_session/` unless the user points you at it. That
  folder is historical context kept for traceability, not current spec.
- Do not read every file under `docs/calib_board_recognize/` — start with
  `calibration_module.md`; descend only if you actually touch that module.
- Do not run `git` write commands, `--no-verify`, or anything destructive
  without explicit user permission.
- Do not invent skill names or doc paths. If a doc is referenced but missing,
  flag it; don't fabricate.

---

## When in doubt

Ask. The user prefers a one-sentence clarifying question over a confidently
wrong implementation. This is rule 11.1 in practice.
