# Planning Archive

This folder keeps implementation plans, handoff notes, and spike reports. The main specification
and public developer docs stay one level up in `docs/`.

## Current Milestone Status

The base serial 6DOF milestone is complete. Phase 9 primitive collision and Phase 10 optional mesh
collision are implemented. The project is ready to be treated as a stable milestone baseline for
future expansion.

Open expansion items:

- Kawasaki RS007N remains blocked until verified dimensions, joint limits, posture rules, and
  source references are provided.
- A user-observed primitive-miss / mesh-hit pose should be captured later to tighten the Nachi mesh
  collision regression test.
- Manual visual QA screenshots for Robot3DVizualize mesh modes can be added when release evidence is
  needed.
- Additional robot families, environment collision, path planning, dynamics, or safety-rated
  collision behavior are future scopes and require explicit approval.

## Files

- [robot_kinematics_implementation_plan.md](robot_kinematics_implementation_plan.md): phase-by-phase
  implementation plan and completion status.
- [collision_detection_plan.md](collision_detection_plan.md): Phase 9 primitive collision plan.
- [collision_detection_handoff.md](collision_detection_handoff.md): Phase 9 handoff notes.
- [mesh_collision_backend_plan.md](mesh_collision_backend_plan.md): Phase 10 mesh backend plan,
  current implementation status, and follow-ups.
- [mesh_collision_backend_handoff.md](mesh_collision_backend_handoff.md): Phase 10 handoff notes.
- [mesh_collision_backend_spike.md](mesh_collision_backend_spike.md): backend dependency spike and
  build evidence.
- [milestone_closeout.md](milestone_closeout.md): current milestone baseline, verification commands,
  and expansion boundaries.

## Rules For Future Plans

- Keep new plan/handoff/spike docs in this folder.
- Keep public API/reference material in `docs/developer-guide/`.
- Keep source-data notes in `docs/preset_references/`.
- Update `README.md`, `docs/developer_onboarding.md`, and `AGENTS.md` when plan paths or milestone
  status change.
