# ADR-0004: Hybrid IK — analytic plugin when supported, numerical fallback

## Status
Accepted

## Date
2026-06-19

## Context
Inverse kinematics has two broad approaches. Closed-form **analytic** solvers are exact, fast,
and enumerate all discrete branches — but each only works for a specific robot morphology.
**Numerical** solvers (e.g. damped least squares) work for any chain but return found solutions
near seeds rather than an exhaustive set, can miss branches, and can stall near singularities.
We want both correctness/coverage for common arms and generality for everything else, behind one
API.

## Decision
`SerialRobotKinematics::solve`/`solveAll` are **hybrid**:

1. If an analytic plugin reports it supports the model (`AnalyticIKSolver::supportsModel`) and it
   returns a solution, use it.
2. Otherwise fall back to the adaptive damped least-squares `NumericalIKSolver`.

The first analytic plugin, `Analytic6DofSphericalWristSolver`, supports articulated arms
(J1 ⊥ J2, J2 ∥ J3, no shoulder offset) with a spherical wrist whose orientation reduces to a
proper Z-Y-Z Euler problem. It returns up to 8 discrete branches with shoulder/elbow/wrist
posture signs. Selection is automatic and transparent to the caller.

## Alternatives Considered
- **Numerical only.** Simpler, but no exhaustive branch enumeration and weaker near
  singularities. Rejected for arms where a closed form exists.
- **Analytic only.** Rejected: doesn't generalize to arbitrary morphologies; the virtual test
  arm and many real robots wouldn't be solvable.
- **Make the caller pick the solver.** Rejected as the default: the facade can detect support
  itself. (Direct solver use is still available via the `Solvers/` interfaces.)

## Consequences
- For supported arms (e.g. `NachiMZ04D`), `solve`/`solveAll` now route through the analytic
  solver and return exact branches; `solve` picks the branch closest to the seed/previous joints.
- For unsupported morphologies (e.g. `Virtual6DofTestArm`), behavior is the numerical solver as
  before.
- `solveAll` coverage differs by solver: exhaustive for the analytic plugin, *found-solutions*
  for the numerical one. Document and respect this (see `conventions-and-gotchas.md`).
- Capability detection geometry is derived from the FK chain, so it tracks the canonical model
  (ADR-0001) rather than assuming a particular DH table.
