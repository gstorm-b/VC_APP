# Technical Debt And Next Steps

**Date:** 2026-06-24  
**Status:** Active implementation backlog after restructure closeout

## Highest Priority Debt

### Runtime/Task Safety

- Fix `TaskLocalization` shared-pointer ownership bugs where raw device
  pointers are reset into independent `std::shared_ptr` control blocks.
- Finish runner-based active-camera switching. The controller guards camera
  changes while a cycle is running, but the broader task path still needs to be
  reviewed so device reconnects are coordinated through runners.
- Decide whether `LocalizationRuntimeController` must move into
  `TaskRunner::m_runtimeThread` or whether the current coordinator-thread model
  is the supported design.
- Snapshot `MatchGroup` data before worker-thread matching so GUI mutation
  cannot race the matching worker.

### Product Verification

- Run an operator UI pass against a real or simulated Localization cycle:
  dashboard lamps, fault panel, KPIs, result table, task-local log, read-only
  dashboard behavior, and recovery messaging.
- Measure runtime matching latency and UI responsiveness. Move matching off the
  controller call stack only if measurement shows it is needed.
- Implement customer installer packaging and run clean-machine smoke
  verification.

### Persistence And Schema

- Add schema/version validation to `TaskLocalizeConfig::toJson()` /
  `fromJson()`.
- Validate imported binding data, especially camera numbers and device-id
  strings.
- Decide whether `docs/architecture_docs/` is regenerated, hand-maintained, or
  removed as stale API reference.

## Medium Priority Debt

- Extract shared gadget meta-property helper logic if property-browser dispatch
  keeps growing.
- Finish QSS token mechanism and migrate remaining hardcoded theme colors.
- Replace remaining `ConnectStatus` `default:` switches where explicit enum
  handling would catch future states.
- Improve user-facing feedback for device rename/save failures.
- Review `TaskLocalization` public limit constants and dead members after
  runtime tests are stable.

## Product/Packaging Next Stages

### Stage A - Operator Runtime Validation

Goal: prove the current single-app Runtime mode before packaging.

Work:

- Build a repeatable simulated cycle scenario if hardware is unavailable.
- Capture screenshots/logs of ready, running, success, and fault states.
- Confirm PLC outputs match `docs/task_localization/plc_signal_contract.md`.
- Record failures in `docs/later_todo_list.md` or dedicated issue docs.

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

- Use `docs/later_todo_list.md` for localized deferred work.
- Use this document for cross-cutting debt that affects release planning.
- Do not mark debt resolved until code, docs, and verification evidence are all
  updated.

