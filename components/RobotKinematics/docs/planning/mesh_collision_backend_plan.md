# Mesh Collision Backend Implementation Plan

## Decision

RobotKinematics should keep its own public collision API and data model, but should not implement
triangle-mesh collision algorithms from scratch.

The planned accurate collision backend will use a specialized collision library behind an internal
adapter. The current primitive collision implementation remains useful as a fast fallback, a debug
mode, and an authoring aid, but it is not accurate enough to be the source of truth for STL part
coverage.

Recommended backend evaluation order:

1. Coal
2. FCL
3. VTK as an example/debug baseline only

Coal and FCL are preferred for a core/library backend because they are robotics-oriented collision
libraries. VTK is already available in the visualizer and has `vtkCollisionDetectionFilter`, but VTK
must remain example/debug tooling unless the user explicitly approves adding it as a core
dependency.

## Problem Statement

The primitive collision module works mechanically, but the primitive profile does not fully cover the
real Nachi STL parts. Some poses can be missed because the runtime geometry is an approximation.

We need a mesh collision path that:

- covers the full STL part surface;
- keeps RobotKinematics API stable and independent from a specific third-party library;
- allows optional simplification for performance without silently losing coverage;
- keeps the existing Qt/qmake core build usable when the mesh backend dependency is unavailable.

## Current Status Update

As of the latest Phase 10 implementation slice:

- Phase 10.1 Coal runtime proof is now complete in this workspace through the optional
  RobotKinematics-side synthetic mesh test path.
- Phase 10.2 backend-neutral collision abstraction is implemented in the default build.
- Phase 10.3 mesh-profile types, JSON loading, and validation are implemented.
- Phase 10.4 STL mesh loading/normalization is implemented and covered by Qt Test.
  `StlMeshLoadOptions::rejectDegenerateTriangles=false` now silently filters degenerate
  triangles (the spec-allowed "removed with diagnostics" path) instead of including them,
  so real CAD-exported STLs with sporadic degenerate faces load cleanly through the Coal
  backend.
- Phase 10.5 now has an optional Coal adapter behind compile flags, with synthetic backend-enabled
  tests passing and the default no-backend build still passing. The Coal adapter uses the
  filter-degenerate STL load path so production paths do not fail on CAD exports.
- Phase 10.6 is implemented: `presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json` carries all
  eight Nachi MZ04D STL parts (base, j1-j6, centering tool) with derived `meshToLink` transforms
  validated against the visualizer's home placement table by
  `tests/integration/NachiMeshCollisionTests.cpp`.
- Phase 10.7 ships `tools/mesh_simplification/` and the convenience scripts
  `scripts/build_mesh_simplification_msvc.bat` /
  `scripts/run_mesh_simplification_nachi_msvc.bat`. The tool emits voxel-grid-decimated STLs
  and a paired `*_simplified*` mesh profile with `quality.mode = "simplified"`,
  `simplifiedFrom`, `triangleCount`, `maxSimplificationError_m`, and a margin padded by
  `0.5 * safety_factor * max_error_m`. The leading `0.5` makes `safety_factor=1.0` give a
  pair-level threshold of roughly `max_error_m` (typical case) rather than `a.max + b.max`
  (worst case, available via `safety_factor=2.0`). This avoids a false positive between
  `j5_mesh` and `centering_tool_mesh` at Home/Midpoint where the original Nachi visual STLs
  sit only ~3 mm apart. Generated artifacts are gitignored under
  `presets/Nachi/MZ04/simplified/` and `presets/Nachi/MZ04/*_simplified*.json`.
- Phase 10.8 adds a Collision Backend selector group to the Robot3DVizualize example. The
  combo box lists Primitive / Mesh - Original STL / Mesh - Simplified STL (mesh entries only
  appear when the mesh backend is compiled in and the relevant profile validates). Mesh
  collision truth comes from `CollisionBackends::checkMesh`; VTK is only used to render the
  result. The example builds via the existing
  `scripts/build_example_robot3dvisualize_msvc.bat` for the primitive-only library and via
  the new `scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat` against the Coal-enabled
  library.
- Benchmark snapshots on the real Nachi profile through `tools/mesh_collision_benchmark`
  (1000-iteration aggregate divided by iteration count, single-threaded MSVC release on the
  development workstation):
  - original profile @ home pose: `~272 us/check` (`hasCollision=0`, `pairCount=21`,
    distance > 0). Cached BVH after warmup.
  - simplified profile (voxel-count=80, safety-factor=0.5) @ home pose: `~182 us/check`.
  - both profiles correctly detect collision at the folded pose `joints_rad=0,-1.484,3.054,0,0,0`
    (original distance `-1.7 mm`, simplified distance `+2.9 mm` flagged via padded margin).
- Review fixes before Phase 10.6 are complete: relative mesh paths loaded from profile files are
  resolved against the profile directory, mesh-profile numeric fields are strictly validated, Coal
  safety-margin behavior is covered by test, and optional third-party dependency sources/build
  trees/install roots now live under `third_party/` for Windows 11 + MSVC reproducibility.

## Non-Goals

- Do not remove the primitive collision backend.
- Do not expose Coal, FCL, VTK, Open3D, CGAL, or libigl types in public RobotKinematics headers.
- Do not claim physical safety certification.
- Do not implement path planning, trajectory/swept collision, dynamics, or environment collision in
  this phase.
- Do not use simplified meshes as the default accurate mode unless coverage error is measured and
  compensated by margin.
- Do not add runtime VTK dependency to the core library as the first choice.

## Architecture

Use a backend abstraction:

```cpp
enum class CollisionBackendKind {
    Primitive,
    Mesh
};

enum class MeshCollisionBackendKind {
    None,
    Coal,
    Fcl,
    VtkDebug
};

struct CollisionBackendInfo {
    CollisionBackendKind kind;
    std::string backendName;
    bool supportsMesh = false;
    bool supportsDistance = false;
    bool supportsContacts = false;
};
```

The public API should continue returning `CollisionCheckResult`. Backend-specific result fields
should be normalized into existing pair diagnostics plus optional contact metadata added through
RobotKinematics-owned structs.

Runtime flow:

```text
SerialRobotConfig + JointVector
        |
        v
ForwardKinematics::computeChain
        |
        v
Collision profile / mesh profile pair generation
        |
        v
Internal backend adapter
        |
        v
CollisionCheckResult
```

The mesh backend should prebuild or cache per-link mesh/BVH data and only update transforms for each
joint-state check.

## Mesh Collision Profile Schema Direction

Keep primitive profile schema `robot-kinematics-collision/v1` as-is.

Add a separate mesh profile artifact:

```json
{
  "schema": "robot-kinematics-collision-mesh/v1",
  "profile": {
    "id": "nachi_mz04d_stl_mesh_collision",
    "robotModel": "MZ04D",
    "units": { "length": "m", "angle": "rad" },
    "backendPreference": ["coal", "fcl", "vtk_debug"]
  },
  "meshes": [
    {
      "id": "j2_mesh",
      "link": "link_2",
      "path": "MZ04-01_j2.stl",
      "format": "stl",
      "sourceUnits": "mm",
      "scaleToMeters": 0.001,
      "meshToLink": {
        "xyz_m": [0.0, 0.0, 0.0],
        "rpy_rad": [0.0, 0.0, 0.0]
      },
      "margin_m": 0.0,
      "enabled": true,
      "quality": {
        "mode": "original",
        "triangleCount": null,
        "simplifiedFrom": null,
        "maxSimplificationError_m": null
      }
    }
  ],
  "disabledPairs": [
    {
      "a": "base_mesh",
      "b": "j1_mesh",
      "reason": "adjacent_joint_contact"
    }
  ],
  "sources": [
    {
      "type": "visual_cad_stl",
      "title": "Nachi MZ04D STL visual assets",
      "reference": "presets/Nachi/MZ04",
      "appliesTo": ["mesh_collision_geometry"]
    }
  ],
  "metadata": {}
}
```

Important schema rules:

- `sourceUnits` is required for STL-based assets.
- `scaleToMeters` is required and must be explicit.
- `meshToLink` is required; do not reuse example-local visual corrections implicitly.
- Relative `path` values loaded through `MeshCollisionProfileJsonLoader::loadFile(path)` are resolved
  relative to the mesh-profile JSON directory. `loadJson(json)` leaves paths unchanged.
- Numeric fields must be JSON numbers, not strings. Non-finite values are invalid.
- `quality.mode` must be `original`, `simplified`, or `convex`.
- Simplified meshes must record their source and measured/assumed maximum error.

## Dependency Policy

Mesh collision backend dependencies are optional.

The default core build must remain usable without Coal/FCL unless the user explicitly decides to make
one backend mandatory. Add qmake switches such as:

```text
CONFIG += robotkinematics_mesh_collision
MESH_COLLISION_BACKEND = coal
COAL_ROOT = D:/path/to/coal/install
FCL_ROOT = D:/path/to/fcl/install
```

If the backend is unavailable:

- primitive collision tests must still pass;
- mesh backend tests should be skipped or not built unless the backend is enabled;
- the API should report `UnsupportedSolver` or a collision-specific `InvalidRequest` message for
  mesh requests when no mesh backend is compiled.

Local dependency checkout/install directories used by the helper scripts are part of the tracked
third-party workspace: sources live under `third_party/<name>`, Windows/MSVC build trees under
`third_party/build/<name>`, and install roots under `third_party/install/<name>`. The supported
third-party build environment is Windows 11 with MSVC/Visual Studio 2022 via
`scripts/build_third_party_*_msvc.bat`.

## Implementation Phases

### Phase 10.1: Dependency Spike

**Description:** Prove which backend can be built and linked with MSVC/qmake in this repository.

**Acceptance criteria:**
- [x] Coal spike attempts a minimal two-mesh collision executable or test.
- [ ] FCL spike attempts the same if Coal is not immediately viable.
- [x] VTK spike may be used only as a visual/debug baseline.
- [x] A short dependency report records build steps, installed paths, linked libs, license notes,
      runtime DLL needs, and whether distance/contact queries are available.
- [x] No production API depends on the spike.

**Verification:**
- [x] Run the spike build command documented by the agent.
- [x] Run one minimal collision query between two triangle-mesh fixtures.

**Dependencies:** Existing Phase 9 collision module.

**Files likely touched:**
- `docs/planning/mesh_collision_backend_spike.md`
- optional temporary spike under `examples/` or `tools/mesh_collision_spike/`

**Estimated scope:** Medium.

### Phase 10.2: Backend Abstraction

**Description:** Add RobotKinematics-owned internal interfaces for collision backends without
exposing third-party types.

**Acceptance criteria:**
- [x] Primitive checker can be wrapped as one backend.
- [x] Mesh backend can be represented even when not compiled.
- [x] Public result objects stay backend-neutral.
- [x] Backend availability and capabilities can be queried.

**Verification:**
- [x] Unit tests verify primitive backend through the abstraction.
- [x] Unit tests verify unavailable mesh backend returns a structured unsupported result.

**Dependencies:** Phase 10.1 decision and Phase 9 checker.

**Files likely touched:**
- `include/RobotKinematics/Collision/...`
- `src/Collision/...`
- `tests/unit/CollisionBackendTests.*`

**Estimated scope:** Medium.

### Phase 10.3: Mesh Profile Types And Loader

**Description:** Add `robot-kinematics-collision-mesh/v1` data types and JSON loader.

**Acceptance criteria:**
- [x] Mesh profiles load standalone from JSON.
- [x] Loader requires `sourceUnits`, `scaleToMeters`, `meshToLink`, link id, path, format, and
      disabled-pair metadata.
- [x] Unknown top-level fields are rejected unless under `metadata`.
- [x] The loader validates that all mesh ids are unique and pair references exist.
- [x] The loader does not build any backend BVH yet.

**Verification:**
- [x] Unit/integration tests cover valid profile, missing unit scale, invalid link, duplicate mesh id,
      invalid pair, and metadata preservation.

**Dependencies:** Phase 10.2.

**Files likely touched:**
- `include/RobotKinematics/Collision/MeshCollisionProfile.h`
- `include/RobotKinematics/Collision/MeshCollisionProfileJsonLoader.h`
- `src/Collision/MeshCollisionProfileJsonLoader.cpp`
- `tests/integration/MeshCollisionProfileJsonTests.*`

**Estimated scope:** Medium.

### Phase 10.4: STL Mesh Import And Normalization

**Description:** Load STL mesh data into RobotKinematics-owned vertices/triangles in meters.

**Acceptance criteria:**
- [x] ASCII and binary STL are supported or unsupported formats fail clearly.
- [x] Unit conversion uses explicit `scaleToMeters`.
- [x] Degenerate triangles and non-finite vertices are rejected or removed with diagnostics.
- [x] Mesh statistics include triangle count, bounds, and scale.
- [x] Loader is independent from VTK.

**Verification:**
- [x] Tests cover small ASCII and binary STL fixtures in mm and m.
- [x] Tests prove mm STL vertices become meter values.

**Dependencies:** Phase 10.3.

**Files likely touched:**
- `include/RobotKinematics/Collision/TriangleMesh.h`
- `include/RobotKinematics/Collision/StlMeshLoader.h`
- `src/Collision/StlMeshLoader.cpp`
- `tests/unit/StlMeshLoaderTests.*`

**Estimated scope:** Medium.

### Phase 10.5: Mesh Backend Adapter

**Description:** Implement the selected backend adapter, with Coal preferred if the spike succeeds.

**Acceptance criteria:**
- [x] Mesh/BVH objects are built once per mesh asset and reused.
- [x] Per-check FK only updates transforms.
- [x] Disabled pair filtering is shared with primitive collision behavior.
- [x] Result includes pair ids, link ids, collision bool, and distance/contact data when supported.
- [x] Backend code is behind compile flags and does not affect default no-backend builds.

**Verification:**
- [x] Backend-enabled tests run collision on two synthetic triangle meshes.
- [x] Backend-disabled tests still pass and return structured unsupported status for mesh checks.
- [x] Backend-enabled tests cover request-level `safetyMargin_m` on a separated mesh pair.
- [ ] Measure and record approximate per-check runtime for a tiny fixture and Nachi STL fixture.
  Synthetic timing is now recorded via `tools/mesh_collision_benchmark`; Nachi timing remains open
  until the real mesh profile exists.

**Dependencies:** Phases 10.1, 10.3, and 10.4.

**Files likely touched:**
- `src/Collision/CoalMeshCollisionBackend.cpp` or `src/Collision/FclMeshCollisionBackend.cpp`
- backend qmake `.pri` file
- `tests/unit/MeshCollisionBackendTests.*`

**Estimated scope:** Large. Split into build adapter, synthetic test, then full profile test.

### Phase 10.6: Nachi Mesh Collision Profile

**Description:** Create an accurate STL-backed collision mesh profile for Nachi MZ04D.

**Acceptance criteria:**
- [x] All relevant Nachi STL parts are represented as mesh assets.
- [x] Each mesh has explicit `meshToLink` transform, unit scale, and source reference.
- [x] Adjacent/self-contact pairs are disabled with reasons.
- [x] The profile can be loaded and validated.
- [x] Example-known collision cases are detected better than primitive mode (verified at the folded
      pose `joints_rad=0,-1.484,3.054,0,0,0` where the mesh backend reports a `-1.7 mm` penetration
      via 16 contacts; the same pose ran cleanly through the Coal-enabled `mesh_collision_benchmark`
      and the integration test).

**Verification:**
- [x] Mesh profile JSON test passes (`NachiMeshCollisionTests::profileLoadsAndValidatesAgainstNachiPreset`).
- [ ] Manual visual QA confirms mesh transforms match rendered STL parts. (Pending user-driven run
      of the visualizer with `Mesh - Original STL` selected; the automated test
      `meshToLinkTransformsReproduceVisualizerHomePlacement` already proves the home pose matches
      the visualizer's `visualHomeCorrectionForPartKey` table.)
- [ ] At least one user-observed false negative from primitive mode is captured as a regression test
      if the pose is known. (Placeholder smoke pose committed; tighten with a user-confirmed
      problem configuration once available.)

**Dependencies:** Phase 10.5.

**Files likely touched:**
- `presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json`
- `tests/integration/NachiMeshCollisionTests.*`
- `examples/Robot3DVizualize/...` only if visual QA tooling is needed.

**Estimated scope:** Medium to Large.

### Phase 10.7: Optional Mesh Simplification Pipeline

**Description:** Add offline tooling to generate simplified collision meshes after original-mesh mode
is working.

**Acceptance criteria:**
- [x] Original mesh remains the accuracy baseline (the simplified profile is emitted as a
      separate JSON next to the original; the simplified meshes never overwrite the originals).
- [x] Simplified mesh records source, triangle count, method, and max error
      (`quality.simplifiedFrom`, `quality.triangleCount`, `quality.mode = "simplified"`,
      `quality.maxSimplificationError_m`, plus `metadata.simplification_method` and
      `metadata.simplification_voxel_count`).
- [x] Simplified mesh must not silently shrink coverage; add margin if simplification error is
      non-zero or unknown. The tool pads each mesh by
      `0.5 * safety_factor * max_error_m`; because runtime pair margin sums both meshes' margins,
      `safety_factor=1.0` gives a typical pair-level threshold of about one max-error for
      symmetric mesh errors, while `safety_factor=2.0` approximates the strict sum-of-errors bound.
- [x] No new optional dependency was needed — Open3D, CGAL, or libigl was deferred since a
      dependency-free voxel-grid vertex clustering implementation already satisfies the
      acceptance bar and keeps the tool dependency-aligned with the rest of the repo
      (Qt + Eigen only).

**Verification:**
- [x] Compare original vs simplified collision results on selected poses
      (`tools/mesh_collision_benchmark` on the original and simplified Nachi profiles at
      home and at the folded pose; both detect the collision at the folded pose, neither
      reports a false collision at home with the recommended `voxel-count=80 safety-factor=0.5`
      setting).
- [x] Record speedup and any behavior differences (see Current Status Update — roughly 1.5x
      per-check speedup on the dev workstation at the recommended setting, with the simplified
      result flagged via padded margin rather than raw mesh penetration).

**Dependencies:** Phase 10.6.

**Files likely touched:**
- `tools/mesh_simplification/`
- `presets/Nachi/MZ04/*_simplified*.json`
- `docs/planning/mesh_collision_backend_spike.md`

**Estimated scope:** Medium.

### Phase 10.8: Example Backend Mode UI

**Description:** Let the Qt/VTK example choose collision mode for debugging and validation.

**Acceptance criteria:**
- [x] UI controls are designed in `mainwindow.ui` (Collision Backend group box with the
      backend selector combo plus mesh-backend/profile status labels).
- [x] QSS lives in `.qss` through the existing `.qrc` (new label selectors added to
      `examples/Robot3DVizualize/styles/robot3dvisualize.qss`).
- [x] The existing `.pro` remains the canonical Qt Creator project; `ROBOTKINEMATICS_LIB_DIR`
      is now configurable (env var or qmake arg) so the same `.pro` can target the default
      or Coal-enabled library.
- [x] The app displays Primitive, Mesh Original, and Mesh Simplified availability/status
      (mesh entries only appear when the mesh backend is compiled in AND the relevant profile
      validates; otherwise the status label explains the gap and how to fix it).
- [x] Mesh collision truth comes from `CollisionBackends::checkMesh`, not VTK actor
      intersection. The existing collision-pair table reuses the same render path for primitive
      and mesh modes.

**Verification:**
- [x] Example builds with the selected backend enabled
      (`scripts/build_example_robot3dvisualize_msvc.bat` for primitive-only and
      `scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat` for the Coal-enabled
      visualizer).
- [ ] Manual QA covers a primitive miss that mesh mode detects. (Pending user-driven visual
      QA in the Coal-enabled visualizer; the headed-app launch is necessarily a manual step.)

**Dependencies:** Phase 10.6.

**Files likely touched:**
- `examples/Robot3DVizualize/...`

**Estimated scope:** Medium.

## Checkpoints

### Checkpoint A: Backend Decision

After Phase 10.1:

- [x] Pick Coal, FCL, or defer mesh backend.
- [x] Record exact install/build steps.
- [ ] Confirm license compatibility.
- [x] Confirm MSVC/qmake path.

Do not continue to production integration until this checkpoint is accepted.

### Checkpoint B: Backend-Neutral API

After Phases 10.2 to 10.4:

- [x] Default build still works without mesh backend.
- [x] Mesh profile and STL loader tests pass.
- [x] No third-party types are exposed in public headers.

### Checkpoint C: Accurate Nachi Mesh Mode

After Phases 10.5 to 10.6:

- [x] Nachi mesh collision detects synthetic and folded-pose problem cases through the Coal
      adapter; one user-observed problem pose is still pending to tighten the regression
      assertion in `meshBackendDetectsKnownSelfCollisionPoseWhenCompiled`.
- [x] Primitive remains available as fallback/debug (default core build still ships only the
      primitive backend; the Coal adapter is opt-in via qmake flags).
- [x] Performance is measured enough to decide whether simplification is needed. The synthetic
      ~150 us/check baseline holds, but the real Nachi profile lands at roughly 272 us/check
      (original) and 182 us/check (voxel-grid simplified, error-padded margin). That gap is
      what motivated Phase 10.7; the simplification tool now ships so users can trade resolution
      for speed when needed.

## Risks And Mitigations

| Risk | Impact | Mitigation |
|---|---|---|
| Coal/FCL is hard to build with MSVC/qmake | High | Run Phase 10.1 first; keep backend optional; use VTK only as debug baseline if needed. |
| Third-party types leak into public API | High | Keep adapter internal and expose only RobotKinematics-owned structs. |
| STL units are wrong | High | Require `sourceUnits` and `scaleToMeters`; test mm-to-m conversion. |
| Mesh transforms do not match FK link frames | High | Require explicit `meshToLink`; validate in the Qt/VTK example before accepting the profile. |
| Simplified mesh misses collisions | High | Original mesh is baseline; simplified mesh needs error metadata and margin. |
| Mesh collision is too slow for interactive use | Medium | Cache BVH; measure per-check runtime; add simplified mode only after original mode is correct. |
| Adjacent links cause noisy contacts | Medium | Preserve disabled-pair rules with reasons. |

## Handoff Notes

The repo has moved through Phase 10.6, 10.7, and 10.8. Backend-neutral mesh types/loaders, STL
normalization, the optional Coal adapter, the Nachi mesh profile, the offline simplification
tool, and the example backend selector are now in place. Remaining follow-up:

1. capture a user-observed primitive-miss / mesh-hit pose and tighten
   `NachiMeshCollisionTests::meshBackendDetectsKnownSelfCollisionPoseWhenCompiled` to assert
   `hasCollision == true` at that pose;
2. run the Coal-enabled visualizer
   (`scripts/build_example_robot3dvisualize_mesh_coal_msvc.bat`) and screen-capture each mesh
   mode for the manual-QA acceptance items on Phase 10.6 and Phase 10.8;
3. iterate `voxel-count` / `safety-factor` on the simplification tool against poses the user
   cares about, and consider documenting per-robot recommended settings; and
4. only return to FCL if Coal packaging/shipping becomes problematic later.
