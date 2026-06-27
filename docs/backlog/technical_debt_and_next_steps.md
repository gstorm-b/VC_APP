# Technical Debt And Next Steps

**Date:** 2026-06-24  
**Status:** Active implementation backlog after restructure closeout

## Highest Priority Debt

### Runtime/Task Safety

- **Done (2026-06-24):** ~~Fix `TaskLocalization` shared-pointer ownership bugs
  where raw device pointers are reset into independent `std::shared_ptr` control
  blocks.~~ Audit found this already fixed — no `.reset(raw)` remains, and device
  typing uses `std::dynamic_pointer_cast`, so every owner shares the single
  `DeviceManager` control block. See `docs/backlog/later_todo_list.md` item 22.1.
- **Done (2026-06-24):** ~~Finish runner-based active-camera switching.~~
  Reviewed the full `TaskLocalization::setCameraNumber` →
  `LocalizationRuntimeController::setActiveCameraNumber` path: it is fully
  runner-coordinated (previous camera disconnects via `requestDisconnect()`, new
  via `requestConnect()`, per-cycle grab/command connections always target the
  active `CameraRunner`, mid-cycle changes are guarded). Fixed one latent
  null-deref/stale-workspace defect on the camera-switch path. See
  `phase2_phase3_runtime_hardening.md` → "Still Open".
- **Done (2026-06-24):** ~~Decide whether `LocalizationRuntimeController` must
  move into `TaskRunner::m_runtimeThread`.~~ Decision recorded: the controller
  runs on the coordinator thread, devices keep per-device threads, and matching
  is offloaded to `matchingRunner`. This is the supported design; revisit only on
  measured latency evidence. See `phase2_phase3_runtime_hardening.md` →
  "Runtime Threading Model Decision (2026-06-24)".
- **Done (2026-06-24):** ~~Snapshot `MatchGroup` data before worker-thread
  matching so GUI mutation cannot race the matching worker.~~ Both paths now hand
  the worker an isolated deep copy via `TaskLocalization::snapshotPatternGroup()`:
  the runtime path in `buildRuntimeContext()`, and the commission path in
  `startCommissionMatching()` (which previously passed the live
  `PatternGroupManager` group straight to the matching thread). Verified by the
  architecture contract suite (38 passed). See `docs/backlog/later_todo_list.md` item 22.6.

### Product Verification

- Run an operator UI pass against a real or simulated Localization cycle:
  dashboard lamps, fault panel, KPIs, result table, task-local log, read-only
  dashboard behavior, and recovery messaging.
- Measure runtime matching latency and UI responsiveness. Matching already runs
  off the controller call stack (on the `matchingRunner` thread); this
  measurement validates that the coordinator-thread model holds under load and
  feeds the threading-model revisit criteria in
  `phase2_phase3_runtime_hardening.md`.
- Implement customer installer packaging and run clean-machine smoke
  verification.

### Persistence And Schema

- **Done (2026-06-24):** ~~Add schema/version validation to
  `TaskLocalizeConfig::toJson()` / `fromJson()`.~~ Added `version` /
  `kSchemaVersion`; documents from a newer app are refused. See
  `later_todo_list.md` 22.8.
- **Done (2026-06-24):** ~~Validate imported binding data, especially camera
  numbers and device-id strings.~~ `TaskDeviceBinding::fromJson()` now
  range-checks camera numbers (1..16) and caps device-id length. See 22.9.
- Decide whether `docs/generated/architecture_docs/` is regenerated, hand-maintained, or
  removed as stale API reference.

## Medium Priority Debt

- **Done (2026-06-24):** ~~Extract shared gadget meta-property helper logic if
  property-browser dispatch keeps growing.~~ Added `vc::gadget_meta` in
  `qgadget_marco.h`; setting + vision widgets route through it. See
  `later_todo_list.md` 22.17.
- **Partially done (2026-06-24):** Finish QSS token mechanism and migrate
  remaining hardcoded theme colors. Added a runtime token resolver: `.qss` files now
  reference `docs/rules/ui_theme_tokens.md` design tokens as `@{group.token}`
  placeholders, and `ThemeManager::resolveTokens()` substitutes them from the
  canonical `tokenTable()` in `theme_manager.cpp` on every sheet load (global apply +
  per-form reload). Migrated all 18 sheets (1054 token references); the sweep is
  provably non-visual-changing — the C++ table reproduces every original colour
  byte-for-byte (modulo case/whitespace). A follow-up handoff sweep then added
  `device.*`, `state.*.bright`, `state.error.deep`, `accent.pressed.deep`, and
  `overlay.*` tokens and migrated the affected QSS selectors onto them. The
  sweep scope is now acceptance-clean except for the approved raw `#7a1010`
  single-use shadow; see `docs/rules/ui_design_rules.md` §3.7 and
  `docs/rules/ui_theme_tokens.md`.
- **Done (2026-06-24):** ~~Finish the remaining UI token closeout for
  `DevicesMonitorWidget` inside `MitsubishiMcDeviceWidget` and
  `SystemLogForm`.~~ Both surfaces now follow the active theme: the PLC monitor
  widget reloads token-backed QSS and delegate painting on theme changes, and
  `SystemLogForm` uses explicit dark/light styling plus visible-entry
  re-rendering so existing log lines recolour after a theme toggle.
- **Done (2026-06-24):** ~~Replace remaining `ConnectStatus` `default:` switches
  where explicit enum handling would catch future states.~~ All `ConnectStatus`
  switches are now exhaustive. See 22.16.
- **Done (2026-06-24):** ~~Improve user-facing feedback for device rename/save
  failures.~~ Rename failures (4 widgets) and the connected-device save block
  now emit a user warning. See 22.21/22.22.
- **Done (2026-06-24):** ~~Review `TaskLocalization` public limit constants and
  dead members after runtime tests are stable.~~ Limits are now
  `static constexpr kLimit*`; dead members removed. See 22.18/22.14.
- **Open (2026-06-24):** Migrate hard-coded local dependency paths in qmake
  files and helper scripts to environment variables. Known touchpoints include
  `ncr_picking.pro`, `tests/architecture_contract_test/architecture_contract_test.pro`,
  generated/legacy `target_wrapper.bat`, and `build_test.bat`. New work must
  follow `docs/rules/build_and_verification.md`: root app builds under root `build\`,
  tests/examples/components build beside their `.pro`, and Qt/OpenCV/Basler/
  Visual Studio paths come from environment variables.

## Product/Packaging Next Stages

### Stage A - Operator Runtime Validation

Goal: prove the current single-app Runtime mode before packaging.

Work:

- Build a repeatable simulated cycle scenario if hardware is unavailable.
- Capture screenshots/logs of ready, running, success, and fault states.
- Confirm PLC outputs match `docs/domains/task_localization/plc_signal_contract.md`.
- Record failures in `docs/backlog/later_todo_list.md` or dedicated issue docs.

Exit gate:

- Operator runtime smoke result is written to docs.
- Any blocking runtime defects have focused tests or reproduction steps.

### Stage B - Installer Prototype

Goal: create an installer or install folder that does not rely on developer
PATH/source-tree state.

Work:

- Pick installer technology.
- Add packaging manifest.
- Add Qt/OpenCV/Basler/RobotKinematics payload rules.
- Include license notices.
- Run missing-DLL analysis on the install folder.

Exit gate:

- Installed app launches on a clean VM.
- `robot_assets/Nachi/MZ04` loads from the install directory.

### Stage C - Release Candidate Hardening

Goal: turn the prototype into a repeatable release candidate.

Work:

- Run architecture contract tests and app build from clean checkout.
- Run operator smoke.
- Run clean-machine install smoke.
- Record dependency versions and rollback plan.

Exit gate:

- Release checklist is complete and does not rely on tribal knowledge.

## How To Track New Debt

- Use `docs/backlog/later_todo_list.md` for localized deferred work.
- Use this document for cross-cutting debt that affects release planning.
- Do not mark debt resolved until code, docs, and verification evidence are all
  updated.
