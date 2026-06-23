# Agent Handoff Notes

## 2026-06-19 Phase 2 Verification And Phase 3+ Continuation

- Current session scope: verify the previously implemented Phase 2 work, then continue the implementation plan in dependency order toward Phase 8 where feasible.
- Build priority: MSVC first. MinGW compatibility can be checked later unless the current task explicitly requires it.
- Test convention: Qt Test classes live in headers and are registered from `tests/TestMain.cpp`; do not include generated moc files at the end of test source files because that breaks the MSVC build.
- Accuracy convention: pose residual targets stay at <= 1e-6 m position and <= 0.001 deg orientation for normal non-singular cases. Joint-vector round-trip comparisons are stricter: <= 0.0001 deg per revolute joint by default, with <= 0.001 deg allowed only for fixtures derived from teach-pendant/reference coordinates rounded to 2 decimal places and documented near the assertion.
- Real robot preset constraint: Kawasaki RS007N remains blocked until verified dimensions, joint limits, posture rules, and source references are provided. Nachi MZ04D is implemented from teach-pendant data; see the later notes in this file.
- Scope discipline: keep core calculations in meters/radians, preserve `Pose` as the canonical transform type, and keep preset data out of solver logic.

## 2026-06-19 Nachi MZ04D Kinematics (Task 6.2, partial)

- Implemented Nachi MZ04D preset (`src/Presets/NachiMZ04D.cpp`, `presets/Nachi/MZ04/nachi_mz04d.json`, header, test `tests/integration/NachiMZ04DTests.cpp`). All suites pass under MSVC.
- Kinematics were reverse-engineered from the teach-pendant measurements in `docs/preset_references/nachi-mz04d.md` (the user's guessed DH was wrong beyond signs). Corrected standard DH and the full derivation are documented at the bottom of that md file. Reproduces all 21 poses to <= 0.035 mm / 0.010 deg.
- CRITICAL convention: the Nachi pendant reports orientation **Z-first** = `(yaw_Z, pitch_Y, roll_X)`. `Pose::fromXYZRPY_*` takes `(roll, pitch, yaw)`, so map the 1st reported number to yaw and the 3rd to roll. This mismatch was the main blocker (position fit while orientation did not).
- Joint limits are applied from the pendant ranges in the md. Posture is mapped from the Nachi manual rules the user added: shoulder=sign(J1) (J1<0 righty, J1>0 lefty), wrist=sign(J5) (J5<0 flip, J5>0 non-flip), elbow=sign(J3) (J3<0 above, J3>0 below). The generic `serial_6dof_shoulder_elbow_wrist` resolver already classifies by sign(J1)/sign(J3)/sign(J5); only the preset `posture.labels` (negative/positive -> name) needed setting. The user's 4-config Arm-config ground-truth set (all below/non-flip, 2 lefty + 2 righty) confirms shoulder/wrist + elbow below-side; elbow above-side inferred. Verified in `NachiMZ04DTests::postureClassificationMatchesNachiManualRules`.
- The same Arm-config set was measured with a tool offset `(44.2, 0, 139.0) mm`; `NachiMZ04DTests::forwardKinematicsWithToolMatchesMeasuredPoses` reproduces those 4 TCP poses to <= 0.04 mm / 0.005 deg, exercising tool/TCP composition. NachiMZ04DTests now has 6 functions; all suites pass under MSVC.

## 2026-06-19 Phase 8 Analytic IK Plugin

- Implemented Task 8.1 + 8.2 in `src/Solvers/Analytic6DofSphericalWristSolver.cpp` (wired into `src/src.pro` and the build). Tests: `tests/unit/AnalyticIKSolverTests.cpp` (5 functions). Kawasaki RS007N (Task 6.1) skipped — no data.
- `supportsModel` now detects the supported morphology by geometry from the FK chain at home: spherical wrist (axes 4/5/6 intersect, via least-squares) + articulated arm (axis1∩axis2, axis1⊥axis2, axis2∥axis3) + a wrist that reduces to a proper Z–Y–Z Euler problem. The old origin-coincidence check wrongly rejected MZ04D (d6=72 offsets J6's frame origin down its axis); fixed.
- `solveAll` is a closed-form decoupled solver (wrist center from a flange-frame constant; q1 + 2R-with-offset for q2,q3; ZYZ wrist for q4,q5,q6), enumerating up to 8 branches, wrapping each joint into limits (prefer natural value), de-duplicating, and FK-residual filtering. Verified vs MZ04D FK to machine precision for all branches (the math was prototyped in Python first). `solve` returns the branch closest to seed/previous.
- `SerialRobotKinematics::run` is now hybrid: analytic when `supportsModel` and it returns a solution, else numerical fallback. Virtual6DofTestArm is non-spherical → numerical (covered by tests). NOTE: MZ04D `solve`/`solveAll` now route through the analytic solver.
- All 20 test suites pass under MSVC (`build_cli` shadow build; run exe from project root).
- Build note: built/ran via `build_cli/` (fresh qmake+jom shadow build); run the test exe from the project root so `presets/...` resolves. The Qt Creator `build/` dir is untouched.
