# Mesh Collision Backend Handoff

## Current Decision

Primitive collision is still the default core build behaviour. On top of that, the repo now
ships an opt-in Coal-backed mesh collision pipeline plus an offline simplification path and an
in-app backend selector for the Robot3DVizualize example. Mesh modes are entirely optional and
only activate when the Coal-enabled library is linked.

Current repo state now includes:

- backend-neutral mesh public types and availability reporting;
- standalone mesh-profile JSON loading and validation;
- STL mesh import with explicit unit normalization (degenerate-triangle filtering is now spec
  compliant: `StlMeshLoadOptions::rejectDegenerateTriangles=false` removes them with diagnostics
  instead of including them, which is what production paths use);
- a VTK debug spike tool and report;
- an optional Coal mesh backend adapter behind compile flags;
- backend-enabled synthetic Qt tests for overlapping and separated triangle meshes;
- backend-enabled safety-margin coverage for a separated synthetic mesh pair;
- a default no-backend path where mesh checks return `UnsupportedSolver`;
- `presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json` covering all eight Nachi MZ04D STL parts
  (base, j1-j6, centering tool) with derived `meshToLink` transforms validated against the
  visualizer's home-pose placement table;
- `tools/mesh_simplification/` plus the convenience scripts
  `scripts/build_mesh_simplification_msvc.bat` and
  `scripts/run_mesh_simplification_nachi_msvc.bat` that emit voxel-grid-decimated STLs and a
  paired simplified profile (margin auto-padded by the measured simplification error);
- a Collision Backend selector group in the Robot3DVizualize example with Primitive /
  Mesh - Original STL / Mesh - Simplified STL modes, including helpful status labels when the
  mesh backend is not compiled in or a profile is missing. The Coal-enabled visualizer is
  built via `scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat`.

Public RobotKinematics APIs should stay project-owned and backend-neutral. Do not expose Coal, FCL,
VTK, Open3D, CGAL, or libigl types in public headers.

## Start Here

Read these files before implementation:

1. `README.md`
2. `docs/robot_kinematics_spec.md`
3. `docs/planning/robot_kinematics_implementation_plan.md`
4. `docs/robot_preset_json_schema.md`
5. `docs/planning/collision_detection_plan.md`
6. `docs/planning/mesh_collision_backend_plan.md`
7. `docs/developer_onboarding.md`
8. `AGENTS.md`

## First Task

Start by reading [docs/planning/mesh_collision_backend_spike.md](/D:/Project/RobotKinematics/docs/planning/mesh_collision_backend_spike.md).

Phases 10.1-10.8 are implemented. The remaining follow-up for the next agent is small but
manual-visual-QA-heavy:

1. capture a real user-observed primitive-miss / mesh-hit pose and tighten
   `tests/integration/NachiMeshCollisionTests.cpp::meshBackendDetectsKnownSelfCollisionPoseWhenCompiled`
   to assert `hasCollision == true` at that pose;
2. build the Coal-enabled visualizer via
   `scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat`, launch it, switch between the
   Primitive / Mesh - Original / Mesh - Simplified modes, and screenshot each so the manual-QA
   acceptance items on Phase 10.6 and Phase 10.8 can be ticked;
3. iterate `voxel-count` / `safety-factor` on `tools/mesh_simplification` for any robot the
   user cares about, and document the recommended settings per robot.

## Backend Preference

Evaluate in this order:

1. Coal
2. FCL
3. VTK debug baseline

Coal/FCL are candidates for core mesh backend. VTK should remain example/debug-only unless the user
explicitly approves a core VTK dependency.

## Hard Rules

- Keep the default core build usable without mesh backend dependencies.
- Use qmake flags or `.pri` files for optional backend integration.
- Keep all mesh data in meters internally.
- STL profiles must explicitly state source units and `scaleToMeters`.
- Mesh transform into a link frame must be explicit as `meshToLink`.
- Relative mesh paths loaded from profile files resolve against the mesh-profile JSON directory.
- Mesh profile numeric fields must be JSON numbers, not strings.
- Do not silently reuse Qt/VTK visual correction code as collision metadata.
- Do not remove primitive collision; it remains fallback/debug.
- Do not claim physical safety certification.
- Optional dependency checkouts, Windows/MSVC build trees, and local installs are intentionally kept
  under `third_party/` so agents can reproduce the mesh-backend environment.

## Dependency Spike Output Template

`docs/planning/mesh_collision_backend_spike.md` now exists and should be kept current with any new Coal/FCL
evidence. The current VTK baseline tool is:

- [tools/mesh_collision_spike/main.cpp](/D:/Project/RobotKinematics/tools/mesh_collision_spike/main.cpp)
- [tools/mesh_collision_spike/mesh_collision_spike.pro](/D:/Project/RobotKinematics/tools/mesh_collision_spike/mesh_collision_spike.pro)
- [scripts/build_mesh_collision_spike_msvc.bat](/D:/Project/RobotKinematics/scripts/build_mesh_collision_spike_msvc.bat)

The dependency report template remains:

```markdown
# Mesh Collision Backend Spike

## Summary

- Recommended backend:
- Result:

## Coal

- Version/source:
- Install/build steps:
- MSVC/qmake integration:
- Minimal collision query:
- Distance/contact support:
- Runtime DLL/library notes:
- License notes:
- Issues:

## FCL

...

## VTK Debug Baseline

...

## Recommendation

[Choose Coal/FCL/defer and why.]
```

## Suggested Spike Fixture

Use two tiny triangle meshes in code or temporary STL files:

- mesh A: one unit square made of two triangles;
- mesh B: same square translated to collide and then separated;
- run collision query in both states;
- if supported, also query distance/contact count.

## Current Local Dependency Roots

The workspace now has reproducible Windows/MSVC install scripts for the mesh-backend prerequisites:

- `scripts\build_third_party_assimp_msvc.bat`
- `scripts\build_third_party_boost_msvc.bat`
- `scripts\build_third_party_libccd_msvc.bat`
- `scripts\build_third_party_fcl_msvc.bat`
- `scripts\build_third_party_coal_msvc.bat`

Installed roots currently available:

- `third_party\install\assimp`
- `third_party\install\boost`
- `third_party\install\libccd`
- `third_party\install\fcl`
- `third_party\install\coal`

These directories are intentionally kept under `third_party/` so the optional mesh-backend
environment can be reproduced on Windows 11 with MSVC. Generated simplified mesh outputs remain
ignored separately under `presets/Nachi/MZ04/simplified/` and
`presets/Nachi/MZ04/*_simplified*.json`.

That closes the old "missing local dependency roots" blocker. The repo now has a tiny Coal runtime
collision proof through the RobotKinematics test suite. A license snapshot is recorded in
`docs/planning/mesh_collision_backend_spike.md`; keep final distribution/legal approval as a release
checklist item rather than an implementation blocker for Phase 10.6.

## Handoff Checklist

When handing off:

- phase/task number;
- backend evaluated;
- exact install path and commands;
- files changed;
- tests or spike commands run;
- result and recommendation;
- known blockers;
- whether the next agent may proceed to Phase 10.6.

Current recommendation:

- backend evaluated: `Coal`
- delivered in this slice: Phase 10.6 Nachi mesh profile + tests; Phase 10.7 simplification tool
  and gitignored simplified-profile output; Phase 10.8 visualizer backend selector and
  Coal-enabled visualizer build script. Benchmark numbers recorded in
  `docs/planning/mesh_collision_backend_plan.md`.
- next task: capture one user-observed problem pose to tighten the regression assertion, then
  run the manual visual QA in the Coal-enabled visualizer.
- known blockers: only manual visual QA and a user-confirmed false-negative pose remain;
  final legal/distribution approval remains a release-management task.
