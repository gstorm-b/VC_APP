# Mesh Collision Backend Spike

## Summary

- Recommended backend: `Coal`.
- Result: Coal is now buildable, linkable, and queryable from RobotKinematics through an optional
  qmake/MSVC adapter path. The local workspace also retains the VTK debug baseline and an FCL build
  fallback, but Coal is the first backend with a real RobotKinematics synthetic runtime proof.

## Commands Run

```powershell
scripts\build_mesh_collision_spike_msvc.bat
D:\Project\RobotKinematics\build\tools\mesh_collision_spike\release\mesh_collision_spike.exe
scripts\build_msvc_mesh_coal.bat
scripts\test_msvc_mesh_coal.bat
scripts\build_mesh_collision_benchmark_msvc.bat
scripts\run_mesh_collision_benchmark_msvc.bat
```

Observed runtime output:

```text
vtk_debug colliding_contacts=52
vtk_debug separated_contacts=0
[OK] VTK debug spike query passed.
Running CollisionBackendTests...
CollisionBackendTests: PASS
[OK] MSVC Coal mesh tests complete.
scenario=synthetic_overlap status=0 iterations=1000 warmup=100 hasCollision=1 pairCount=1 contactCount=16 distance_m=-0.1 total_ms=141.204 avg_us=141.204
scenario=synthetic_separated status=0 iterations=1000 warmup=100 hasCollision=0 pairCount=1 contactCount=0 distance_m=1.23794 total_ms=151.369 avg_us=151.369
[OK] MSVC mesh collision benchmark run complete.
```

## Coal

- Version/source: official git clone at `third_party/coal`, current local HEAD `507bb715`.
- Install/build steps:
  - `git pull` (already up to date in this workspace) for `third_party/coal`
  - `git submodule update --init --recursive` inside `third_party/coal`
  - build prerequisites first:
    - `scripts\build_third_party_assimp_msvc.bat`
    - `scripts\build_third_party_boost_msvc.bat`
    - `scripts\build_third_party_libccd_msvc.bat`
    - `scripts\build_third_party_fcl_msvc.bat`
  - then run `scripts\build_third_party_coal_msvc.bat`
  - install root: `third_party\install\coal`
- MSVC/qmake integration:
  - optional build wiring now lives in `mesh_collision_backend.pri`
  - RobotKinematics-side verification scripts:
    - `scripts\build_msvc_mesh_coal.bat`
    - `scripts\test_msvc_mesh_coal.bat`
    - `scripts\build_mesh_collision_benchmark_msvc.bat`
    - `scripts\run_mesh_collision_benchmark_msvc.bat`
  - compile flag path:
    - `CONFIG+=robotkinematics_mesh_collision`
    - `MESH_COLLISION_BACKEND=coal`
    - `COAL_ROOT=...`
    - `BOOST_ROOT=...`
    - `ASSIMP_ROOT=...`
- Minimal collision query:
  - proven through the RobotKinematics `CollisionBackendTests` synthetic mesh fixture
  - overlapping case: collision detected, `distance_m = -0.1`, `contactCount = 16`
  - separated case: no collision, `distance_m = 1.23794`, `contactCount = 0`
- Distance/contact support:
  - contact counts are available
  - signed distance is available for the exercised synthetic fixture after normalizing Coal contact
    data into RobotKinematics pair results
- Runtime measurement:
  - synthetic overlap fixture: about `141.204 us/check` over 1000 measured iterations after 100 warm-up iterations
  - synthetic separated fixture: about `151.369 us/check` over 1000 measured iterations after 100 warm-up iterations
  - Nachi STL benchmark remains pending until the real mesh profile is authored
- Runtime DLL/library notes:
  - `third_party\install\coal\bin\coal.dll`
  - depends on local `Assimp` and `Boost` DLLs being discoverable at runtime
  - `scripts\build_msvc_mesh_coal.bat` and `scripts\test_msvc_mesh_coal.bat` prepend the required
    `Qt`, `Coal`, `Assimp`, and `Boost` runtime paths
- License notes: local `third_party/coal/LICENSE` is BSD-style. Binary redistribution must preserve
  the license/copyright/disclaimer notices from Coal and its runtime dependencies.
- Issues:
  - local Boost 1.87 install needed two Windows-specific workarounds for Coal:
    - `Boost_INCLUDE_DIR` must point to `third_party\install\boost\include\boost-1_87`
    - `BOOST_INSTALL\lib` must be available on the linker search path because Coal/Boost autolink may
      request `libboost_*.lib` names in addition to explicit full-path import libs
  - the Boost helper script now creates `libboost_*.lib` aliases for reproducibility

## FCL

- Version/source: official git clone at `third_party/fcl`, current local HEAD `e5efcc4`.
- Install/build steps:
  - `git pull` (already up to date in this workspace) for `third_party/fcl`
  - `scripts\build_third_party_libccd_msvc.bat`
  - `scripts\build_third_party_fcl_msvc.bat`
  - install root: `third_party\install\fcl`
- MSVC/qmake integration: local install/build proof now exists; qmake integration for the actual
  RobotKinematics backend is still pending.
- Minimal collision query: not run yet against FCL itself; build proof only.
- Distance/contact support: not measured locally yet in a RobotKinematics spike.
- Runtime DLL/library notes:
  - static `fcl.lib` install was produced in `third_party\install\fcl\lib`
  - depends on local `libccd` install root via CMake/package config during build
- License notes: local `third_party/fcl/LICENSE` is BSD-style. FCL remains fallback-only for now.
- Issues: FCL remains the fallback candidate, but there is still no local synthetic mesh spike or
  RobotKinematics adapter proof yet.

## VTK Debug Baseline

- Version/source: local external install rooted at `D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs` via `VTK_ROOT`.
- Install/build steps:
  - `VTK_ROOT` can come from the environment or default to `D:\Project\vtk_build\vtk\install-x64-cuda-qt-vs`.
  - Build with `scripts\build_mesh_collision_spike_msvc.bat`.
- MSVC/qmake integration:
  - Reuses the repository's existing VTK qmake wiring from [examples/Robot3DVizualize/vtk_config.pri](/D:/Project/RobotKinematics/examples/Robot3DVizualize/vtk_config.pri).
  - The spike project lives at [tools/mesh_collision_spike/mesh_collision_spike.pro](/D:/Project/RobotKinematics/tools/mesh_collision_spike/mesh_collision_spike.pro).
- Minimal collision query:
  - Implemented in [tools/mesh_collision_spike/main.cpp](/D:/Project/RobotKinematics/tools/mesh_collision_spike/main.cpp).
  - Uses two triangle-filtered cube meshes and `vtkCollisionDetectionFilter`.
  - One overlapping transform produced `52` contacts.
  - One separated transform produced `0` contacts.
- Distance/contact support:
  - Contact count is available through `vtkCollisionDetectionFilter::GetNumberOfContacts()`.
  - Gap distance was not measured in this spike; VTK baseline is still debug-only.
- Runtime DLL/library notes:
  - Requires `VTK_ROOT\bin` on `PATH`.
  - Verified local libraries include `vtkFiltersModeling-9.6(.lib/.dll)` and the existing example's baseline VTK modules.
- License notes:
  - `vtkCollisionDetectionFilter.h` in the local install is marked BSD-3-Clause.
- Issues:
  - VTK works as a debug baseline, but it is not approved as the core mesh backend dependency.
  - This spike must not be treated as permission to pull VTK into the core library build.

## Recommendation

Proceed with Coal as the production adapter path while keeping the default build mesh-backend-free.

Next decision points:

1. Record basic runtime measurements for the synthetic fixture and a first real Nachi mesh fixture.
2. Add the real Nachi STL mesh collision profile and fixture coverage on top of the optional Coal
   adapter.
3. Keep FCL as fallback only if Coal integration or packaging later becomes a problem.

Until then, the repo is in a good intermediate state:

- Phase 10.2 backend-neutral public mesh types are implemented.
- Phase 10.3 mesh profile load/validation is implemented.
- Phase 10.4 STL mesh import/normalization is implemented (the production `rejectDegenerateTriangles=false`
  path now silently filters degenerate triangles to keep real CAD STL imports working).
- Phase 10.5 optional Coal adapter is implemented and synthetic backend-enabled tests pass.
- Synthetic runtime measurements are recorded through `tools/mesh_collision_benchmark`.
- Default builds still work without any runtime mesh backend.

## Nachi Mesh Profile Benchmark Snapshot

Once Phase 10.6 was authored, the benchmark tool was also run against the real Nachi STL profile
(1000-iteration aggregate, divided by iteration count, single-threaded MSVC release):

| Profile                                                       | Pose                            | hasCollision | Pair count | Distance (m) | Avg us/check |
| ------------------------------------------------------------- | ------------------------------- | ------------ | ---------- | ------------ | ------------ |
| `presets/Nachi/MZ04/nachi_mz04d_mesh_collision.json`          | home (0,0,0,0,0,0 rad)          | 0            | 21         | 0.11350      | ~272         |
| same, simplified (voxel-count=80, safety-factor=0.5)          | home                            | 0            | 21         | 0.11351      | ~182         |
| original                                                      | folded (0,-1.484,3.054,0,0,0)   | 1 (16 contacts) | 21      | -0.00169     | ~195         |
| simplified (voxel-count=80, safety-factor=0.5)                | folded (0,-1.484,3.054,0,0,0)   | 1 (16 contacts) | 21      | +0.00294     | ~180         |

The simplified profile flags the folded-pose collision via the padded margin even though the raw
mesh distance is slightly positive after voxelisation. That is the intended behaviour: original
mesh remains the accuracy baseline, simplified mesh must not silently shrink coverage.

## Mesh Simplification Tool

`tools/mesh_simplification/main.cpp` implements dependency-free voxel-grid vertex clustering and
emits both a simplified binary STL per mesh and a paired `*_simplified*` mesh-profile JSON. The
profile records `quality.mode="simplified"`, `simplifiedFrom`, `triangleCount`,
`maxSimplificationError_m`, and pads each mesh's `margin_m` by
`0.5 * safety_factor * max_error_m`. The runtime backend pair margin is the sum of both
meshes' margins, so the effective pair-level threshold is approximately
`safety_factor * max_error_m` for symmetric mesh errors:

- `safety_factor=0.0` → no margin padding (only safe if downstream code already handles the
  simplification-error budget).
- `safety_factor=1.0` (default) → pair threshold equal to the larger mesh's
  `max_error_m`. Typical fit for visual STL packs whose surfaces are well separated except at
  intentional contact regions.
- `safety_factor=2.0` → pair threshold equal to the strict sum `a.max + b.max`. Use this for
  the worst-case guarantee that no simplified-vs-original collision is missed.

Open3D, CGAL, and libigl remain available as future tooling improvements but were deliberately
deferred so the tool keeps the same Qt + Eigen dependency footprint as the rest of the repo.

Convenience runner: `scripts/run_mesh_simplification_nachi_msvc.bat [voxel-count] [safety-factor]`.
The simplified STLs land under `presets/Nachi/MZ04/simplified/<robot>/` and the simplified
profile next to the original; both are gitignored to keep generated artifacts out of source
control.

### Calibration note (Nachi)

The Nachi visual STL pack has `j5_mesh` and `centering_tool_mesh` only ~3 mm apart at Home/Midpoint
joint states even in the original profile. With the old `paddedMargin = safety_factor * max_error_m`
formula and `safety_factor=1.0`, the simplified pair margin at `voxel-count=40` (`max_error ≈
2.2 mm` for each mesh) was ~4.4 mm, which produced a false positive between `j5_mesh` and
`centering_tool_mesh` at Home/Midpoint. The current `0.5 *` factor in the padding formula brings
the same setup down to ~2.2 mm pair margin and clears the false positive while still padding for
typical simplification error.

Additional local dependency license snapshot:

- Assimp: local `third_party/assimp/LICENSE` is BSD-style.
- Boost: local `third_party/boost/LICENSE_1_0.txt` is Boost Software License 1.0.
- libccd: local `third_party/libccd/BSD-LICENSE` and README identify libccd as 3-clause BSD. The
  libccd source tree also contains LGPL/GPL test-suite helper files under `src/testsuites/cu`; those
  are not part of the installed/runtime library path used by the current build scripts.

This snapshot is an engineering dependency note, not final legal approval. Before distributing
RobotKinematics binaries with the optional Coal backend, prepare third-party notices for Coal, FCL
if used, Assimp, Boost, libccd, and any transitive DLLs actually shipped.
