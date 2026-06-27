# Phase 0 Documentation Audit

**Date:** 2026-06-23  
**Scope:** documentation/source-of-truth cleanup before larger refactoring

## Completed In This Pass

- Created `docs/architecture/project_restructure_plan.md` as the staged restructure roadmap.
- Updated `uml/README.md` to describe the current `components/RobotKinematics`
  component instead of the removed standalone `rkin` module.
- Updated `docs/generated/architecture_docs/ITask.md` to use the current task lifecycle
  and `TaskState` enum.
- Updated `uml/06_ui_widgets.puml` to reflect implemented working ROI and
  condition ROI behavior.
- Updated the `CameraWorkspace` source comment so it no longer contradicts
  `LocalizationPipeline`.
- Updated the RobotKinematics comment in `ncr_picking.pro` to match the current
  build-folder deploy behavior.
- Marked the stale `TaskLocalization::matchingRunner` teardown TODO as
  resolved because the destructor now quits, waits, and deletes the thread.
- Added a partial-status note to the queued matching metatype TODO: several
  original types are now registered, but `CameraWorkspace` still needs explicit
  verification before the item can be closed.

## Drift Fixed

- Old `ITask` docs listed `Idle`, `Running`, `Paused`, `Stopped`, `Error`, and
  `Completed`; source now uses `Idle`, `CommissionStarting`, `Commission`,
  `RuntimeStarting`, `Ready`, `RunningCycle`, `Recovering`, `Faulted`, and
  `Stopping`.
- `uml/README.md` still treated `rkin` as the active kinematics module even
  though the active component is `components/RobotKinematics`.
- UI UML said runtime ROI application and world-coordinate offset were deferred
  to the old ROI TODO; `LocalizationPipeline` now applies condition ROI through
  `ImageMatcher::setMatchingConditionROI()`, and match results carry
  `cropOffsetPoint`.

## Deletion Candidates

These were not deleted in this pass because they may still carry traceability
or may be generated from a canonical source. Confirm ownership before removal.

- `docs/domains/task_localization/*.html`: likely generated mirrors of Markdown/module
  specs.
- `docs/generated/html/calibration_module.html`: likely generated from
  `docs/domains/calibration/calibration_module.md`.
- `docs/calib_board_recognize/`: deleted after preserving the useful
  calibration-board watch-list in `docs/domains/calibration/calibration_module.md`.
- `docs/history/old_session/`: historical summaries; keep unless the project adopts an
  archive policy outside active docs.

## Open Decisions

- Decide whether the first release is one application with Commission/Runtime
  modes or separate configuration/runtime executables.
  - **Closed 2026-06-24:** first release remains one application with explicit
    Commission/Runtime modes. See
    [phase4_product_release_plan.md](../../product/phase4_product_release_plan.md).
- Decide whether `docs/generated/architecture_docs/` remains hand-maintained API
  reference, is regenerated, or is removed after the current docs are repaired.
- Decide the QSS token implementation mechanism before sweeping hardcoded theme
  colors.
- Decide installer layout for Qt/OpenCV/Basler dependencies, RobotKinematics
  DLLs, and `robot_assets/`.
  - **Partially closed 2026-06-24:** payload and clean-machine gate are defined
    in [customer_installer_packaging.md](../../product/customer_installer_packaging.md);
    installer technology and implementation remain open.
