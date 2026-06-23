# Restructure Closeout

**Date:** 2026-06-24  
**Status:** Initial restructure roadmap closed; implementation roadmap continues

## What Was Completed

### Phase 0 - Documentation And Governance

- Created the staged restructure plan.
- Repaired high-confidence documentation drift in UML/API references.
- Preserved useful calibration-board knowledge, then removed the obsolete
  `docs/calib_board_recognize/` folder.
- Established backlog status conventions and deletion policy.

### Phase 1 - Baseline Verification

- Documented the qmake + MSVC build flow in
  [build_and_verification.md](build_and_verification.md).
- Rebuilt and ran `tests/architecture_contract_test`; exit code `0`.
- Rebuilt the full Debug app target.
- Fixed missing RobotKinematics transitive DLL deployment for build-folder runs.
- Added queued-metatype protection for runtime matching payloads.

### Phase 2 - Runtime Hardening

- Added architecture contract tests for:
  - trigger edge / held-trigger / trigger reset behavior;
  - grab timeout fault `102`;
  - VisionOutput send failure fault `201`;
  - invalid pattern fault `400`;
  - invalid calibration fault `401`;
  - valid PLC `M` bit and `D` word output writes;
  - invalid PLC tag rejection through `PlcRunner`;
  - active camera loss during `RunningCycle`.

### Phase 3 - Scoped Cleanup

- Hardened `IDeviceWidget` with an explicit virtual destructor and
  `Q_DISABLE_COPY_MOVE`.
- Replaced unsafe concrete widget downcasts in Basler and Vision TCP/IP widgets
  with guarded `qobject_cast`.
- Null-initialized typed widget pointers and guarded Vision TCP/IP config save.

### Phase 4 - Product Release Planning

- Chose single-app first release with explicit Commission/Runtime modes.
- Created the customer installer packaging checklist.
- Defined the clean-machine release gate and deferred two-executable split.
- Created root [AGENT.md](../AGENT.md) as the new agent entrypoint.

## Verification Evidence

- `architecture_contract_test.exe -silent` passed with exit code `0` after the
  Phase 2/3 changes.
- Full Debug app build passed after the Phase 2/3 changes.
- RobotKinematics build-folder deployment copies the required DLLs and
  `robot_assets/Nachi/MZ04` tree.

## What This Closeout Does Not Mean

The project is not "done." This closes the initial restructure roadmap and
turns the work into a concrete implementation backlog. The remaining work is
now narrower and more explicit: runtime operator validation, installer
implementation, schema hardening, UI cleanup, and selected lifecycle fixes.

## Canonical Entry Points After Closeout

- [../AGENT.md](../AGENT.md) for new agents joining the project.
- [build_and_verification.md](build_and_verification.md) for qmake/MSVC build
  and test flow.
- [phase4_product_release_plan.md](phase4_product_release_plan.md) for product
  packaging decisions.
- [technical_debt_and_next_steps.md](technical_debt_and_next_steps.md) for the
  implementation backlog after this restructure.

