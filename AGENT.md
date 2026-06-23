# AGENT.md

This is the first file an AI/code agent should read when joining
`ncr_picking`.

## Project Snapshot

`ncr_picking` is a Qt/C++ industrial vision-picking application. The first
commercial path is one application with explicit Commission and Runtime modes,
centered on `TaskLocalization`.

First-release integrations:

- Basler GigE camera.
- Mitsubishi MC PLC.
- VisionOutput TCP/IP server/client.
- Advisory RobotKinematics checking for outgoing pick poses.

Deferred integrations include robot communication runner, Huayan robot,
VisionSerial, BaslerUSB, Realsense, custom calibration-board authoring, and the
customer installer implementation.

## Read First

For most work, read only what matches your task. Do not load the whole docs
tree by default.

1. `docs/design_rules.md` for process, ownership, language, architecture, and
   coding conventions.
2. `docs/build_and_verification.md` for the qmake/MSVC build and test flow.
3. `docs/project_restructure_plan.md` for the closed restructure roadmap and
   source-of-truth hierarchy.
4. `docs/restructure_closeout.md` for what has already been completed.
5. `docs/technical_debt_and_next_steps.md` for the active implementation
   backlog after restructure closeout.
6. `docs/later_todo_list.md` before flagging or fixing known deferred work.

Topic-specific docs:

- Localization runtime: `docs/task_localization/` and
  `docs/task_localization_implementation_plan.md`.
- RobotKinematics integration: `docs/robot_kinematics_module.md`.
- Product packaging: `docs/phase4_product_release_plan.md` and
  `docs/customer_installer_packaging.md`.
- UI/QSS: `docs/ui_design_rules.md`.
- Current UML: `uml/`.

Historical docs:

- `docs/old_session/` is traceability only. Do not treat it as current spec
  unless the user explicitly points there.
- `docs/architecture_docs/` is API reference only and may drift; prefer source
  and tests when behavior matters.

## Operating Rules

- Code, comments, log strings, and committed docs are English.
- User conversation may be Vietnamese when the user writes Vietnamese.
- Prefer small, verifiable slices over broad rewrites.
- Keep unrelated user changes intact. Do not revert dirty work you did not
  create.
- For explicit implementation requests, proceed with the work and keep the user
  updated. For broad, risky, or ambiguous architecture changes, restate the
  scope and ask before editing.
- If you discover unrelated debt, document it in `docs/later_todo_list.md` or
  `docs/technical_debt_and_next_steps.md` instead of silently mixing it into the
  current change.
- Do not add backwards-compatibility shims unless the user explicitly asks. The
  project has not shipped to customers yet.

## Architecture Guardrails

- Source code and tests are the highest source of truth for implemented
  behavior.
- `TaskLocalization` owns persistent task/config concerns. Runtime orchestration
  belongs in `LocalizationRuntimeController` and per-device runners.
- Do not call device methods directly across runtime threads. Use
  `CameraRunner`, `PlcRunner`, `VisionOutputRunner`, or a new runner for the
  relevant family.
- Device families follow abstract base plus concrete subtype:
  `CameraDevice`, `PlcDevice`, `VisionOutputDevice`, `RobotDevice`.
- Every new device subtype must update enum/string conversion, factory
  dispatch, UI dispatch, persistence, and tests.
- UI structure belongs in `.ui`, behavior in `.cpp`, and styling in `.qss`.
  Avoid inline `setStyleSheet()` unless the UI rules explicitly allow it.
- Update `uml/` and docs when changing architecture boundaries.

## Build And Test

This is a qmake project. In command-line MSVC verification, use qmake first and
`nmake /nologo` as the reliable fallback.

Known-good baseline:

- Qt: `C:\Qt\6.8.2\msvc2022_64`
- MSVC: Visual Studio 2022, initialized by `vcvars64.bat`
- OpenCV: `C:\opencv\build`
- Basler Pylon: `C:\Program Files\Basler\pylon`

Important:

- Call `vcvars64.bat` directly. Do not redirect its output.
- Run `qmake` from the build directory before compiling.
- For `tests/architecture_contract_test`, run
  `nmake /nologo -f Makefile.Debug compiler_moc_source_make_all` before the
  normal `nmake /nologo`.
- Use `QT_QPA_PLATFORM=minimal` when running Qt tests headlessly.
- Qt Creator may use `jom`, but from Codex CLI use the qmake + nmake fallback
  unless the `jom` PATH inheritance issue has been solved.

See `docs/build_and_verification.md` for exact commands.

## Current Release Decision

Phase 4 chose a single-app first release:

- one `ncr_picking.exe`;
- explicit Commission and Runtime modes;
- no separate runtime executable until the operator flow is validated.

Customer installer work is still open. Build-folder deployment copies
RobotKinematics DLLs and `robot_assets/`, but a customer installer must have
its own manifest and clean-machine smoke test.

## Highest Priority Next Work

Start with `docs/technical_debt_and_next_steps.md`. The most important next
slices are:

- operator runtime validation with a real or simulated Localization cycle;
- customer installer prototype and clean-machine smoke verification;
- `TaskLocalization` ownership/threading debt around active-camera switching
  and shared device pointers;
- schema/version validation for task config and imported bindings;
- QSS token mechanism and remaining UI cleanup.

## Do Not

- Do not use `docs/old_session/` as current design guidance.
- Do not treat `QMAKE_POST_LINK` build-folder copying as customer packaging.
- Do not mark roadmap items verified without build/test/manual evidence.
- Do not run destructive git or filesystem commands without explicit user
  permission.
- Do not create broad abstractions before a second concrete implementation
  proves the shape.

