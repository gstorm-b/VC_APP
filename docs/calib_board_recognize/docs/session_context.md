# Session Context — Calibration Module Build Log

Snapshot of the design conversation that produced the `calib_board_recognize` module. Useful for future contributors (or future Claude sessions) to understand *why* the code looks the way it does, not only *what* it does. Pair with [`calibration_module.md`](calibration_module.md) for the API reference.

- **Session dates**: 2026-05-21 → 2026-05-22
- **Driver**: ncrn39@gmail.com
- **Working directory**: `c:\DGB\Project\calib_board_recognize`
- **Toolchain**: Qt 6.8.3 (MSVC 2022), OpenCV 4.11 (`opencv_world4110`), C++17
- **Workflow rhythm**: user runs build + tests locally and pastes the output into [`description.md`](../description.md); each round refines the design from feedback.

---

## 1. Final deliverable (state at end of session)

```
calib_board_recognize/
├── calibration/
│   ├── calibration_board.h            — abstract base class + writePrintablePng() free helper
│   ├── calibration_board.cpp
│   ├── fanuc_irvision_board.h         — concrete subclass: Fanuc iRVision dot grid
│   ├── fanuc_irvision_board.cpp
│   ├── calibration_board_factory.h    — CalibrationBoardFactory (allocates by type / preset)
│   ├── calibration_board_factory.cpp
│   ├── calibrator.h                   — planar homography image px <-> robot mm
│   └── calibrator.cpp
├── tests/
│   ├── tests.pro
│   └── tst_main.cpp                   — TestFanucIRvisionBoard, TestCalibrationBoardFactory, TestCalibrator
├── main.cpp                           — end-to-end demo: factory → generate → print → detect → calibrate → save
├── calib_board_recognize.pro
├── description.md                     — running spec (input from user)
└── docs/
    ├── calibration_module.md          — API reference for embedding into other projects
    ├── calibration_module.html        — same content, standalone HTML
    └── session_context.md             — this file
```

Status at session end: **app + tests both green**, main demo produces `board.png`, `board_print_300dpi.png` (with pHYs chunk for 1:1 printing), `board_detected.png`, and `calibration.yml`.

---

## 2. Module summary at a glance

- **Scope**: planar (homography-based) 2D pick-and-place. Full 3D camera calibration was explicitly deferred by the user.
- **Two top-level classes**:
  - `CalibrationBoard` (abstract) — interface for any board family that can synthesise an image, detect itself on a frame, and expose object-point geometry in mm.
  - `Calibrator` — accumulates image↔robot correspondences and fits a homography via `cv::findHomography`. Supports YAML save/load.
- **Factory**: `CalibrationBoardFactory` allocates concrete boards by type enum, by explicit params, or by string preset name (`"iRvision-5mm"`, `"iRvision-11.5mm"`, `"iRvision-15mm"`, `"iRvision-22.5mm"`, `"iRvision-30mm"`).
- **Only concrete subclass today**: `FanucIRvisionBoard`. Pattern: `R×C` odd-sized dot grid with 4 dot categories (Normal solid / Target ring×8 / Centre ring×1 / Coord solid×3). The 4 large markers (1 centre ring + 3 coord) break rotational symmetry and are used for orientation detection.
- **Printable output**: `writePrintablePng()` patches a PNG `pHYs` chunk into OpenCV-encoded PNG bytes so printers reproduce the board at exact mm scale.

---

## 3. Chronological design iterations

The pattern design changed several times as the user clarified intent. Each round was driven by a paste of build / run output, so this section also captures the failure modes that pushed each redesign.

### Round 1 — initial 3-fiducial design
- Requirements (from `description.md`): `CalibrationBoard` capable of generating Fanuc-like image + recognising it; `Calibrator` for planar 2D pick-and-place; Qt Test; main.cpp demo.
- Confirmed: planar 2D only, Qt Test (no GoogleTest), real Fanuc board target. Parameters: rows/cols/dot size/spacing in mm.
- First implementation used **3 different-sized fiducials** at `(0,0)`, `(0,1)`, `(1,0)` to break rotational symmetry; detected with `SimpleBlobDetector`.
- All `TestCalibrationBoard` tests passed first try. `TestCalibrator` crashed on `calibrate_perfectAffine_lowError`.

### Round 2 — std::normal_distribution debug-assert
- Crash root cause: MSVC debug runtime asserts inside `std::normal_distribution<float>` when `stddev == 0`. The `applyAffine` helper in tests passed `noiseStd = 0.0` for the noise-free cases.
- Fix in `tst_main.cpp`: gate sampling on `noiseStd > 0`, return raw `0.0f` otherwise.
- All 8 tests now passed; main demo also passed.

### Round 3 — 9 corner/edge ring markers
- User asked: odd `rows`/`cols`; replace the 3 size-fiducials with **9 ring markers** (4 corners + 4 edge midpoints + 1 centre); use 4 corners for robot calibration in practice.
- Implementation: ring markers with `findContours(RETR_CCOMP)` hierarchy-based ring/solid classification; orientation derived from convex hull of the 9 markers.

### Round 4 — pattern inversion
- User flipped the role assignment: all "regular" dots become rings with white inner (= tool-tip targets); the **4 solid markers** (1 centre + 2 right of centre for +X + 1 above centre for +Y) become orientation-only.
- Implementation pivoted to: ring grid + 4 solid orientation markers, identified by colinearity (`identifyOrientationMarkers4`).

### Round 5 — final 4-category pattern
- User clarified the final pattern format:
  - **Normal** (solid small): fills the grid
  - **Target** (ring small, 8 dots): 4 corners + 4 edge midpoints; the white inner is the tool-tip target
  - **Centre** (ring large, 1 dot): bigger ring at grid centre
  - **Coord** (solid large, 3 dots): 2 right + 1 above centre, encode +X and +Y axes
- Validation tightened: `rows ≥ 5`, `cols ≥ 7` (so coord dots don't collide with edge-midpoint targets).
- Default params: `normalDotSize=4mm`, `coorDotSize=8mm`, `innerTargetDotSize=1mm`, `dotSpacing=15mm`.

### Round 6 — detection failure on iRvisionPattern5
- User added a preset constant `iRvisionPattern5` (17×17 grid, 5mm spacing, 2mm dots, 4mm coord, 0.5mm inner) and ran the demo.
- Detection returned **0/289 dots**. Two root causes:
  1. The `pxPerMm` heuristic assumed the board fills 60 % of the image diagonal — false for this preset (board fills ~90 %), so the area-based small/large split was off and normal dots were misclassified as coord-large.
  2. With the inner white at 5 px, `findContours` sometimes failed to detect the hole reliably, breaking the ring/solid classification.
- Refactored `detect()`:
  - Drop the `pxPerMm` heuristic entirely. Collect every top-level circular contour with no a-priori size filter.
  - Sort blobs by area descending; the **top 4 = the 4 orientation markers** (1 centre + 3 coord, all sized `coorDotSize`).
  - Identify centre / xMid / xTip / yTip purely from geometry: most-colinear triple → off-line marker is `yTip`; middle of the colinear triple is `xMid`; closer-to-`yTip` endpoint is `centre`.
  - Build a per-step basis `vCol = (xTip − centre)/2`, `vRow = centre − yTip` and snap every blob to its nearest grid cell.
- All tests passed; demo ran clean. Mean reprojection error matched the simulated noise level (~0.07 mm at injected 0.05 mm σ).

### Round 7 — printable PNG (1:1 mm scale)
- Added free helper `writePrintablePng(path, img, pxPerMm)` and method `CalibrationBoard::writePrintableImage(path, dpi=300)`.
- Implementation note: OpenCV's `cv::imwrite` does **not** emit a `pHYs` chunk. The helper encodes the PNG via `cv::imencode`, scans the chunk stream for the first `IDAT`, and **inserts a properly-CRC32-checksummed `pHYs` chunk** just before it. Hand-rolled CRC32 (poly `0xEDB88320`) because we don't want to pull in libpng.
- Test `writePrintableImage_embedsPhysChunk` walks the PNG chunks and verifies the chunk exists, sits before `IDAT`, has `unit == 1` (meter), and `ppx ≈ ppy ≈ dpi/25.4 × 1000`.

### Round 8 — first docs pass
- Generated `docs/calibration_module.md` and a self-contained `docs/calibration_module.html` (no external CSS/JS) intended as embeddable reference material for downstream projects / future Claude sessions.

### Round 9 — abstract / factory refactor
- User asked: make `CalibrationBoard` an **abstract base class** so other board families (Halcon, ChArUco, …) can be added later; move the current implementation into a concrete subclass; add a factory.
- Split into:
  - `calibration_board.{h,cpp}` — pure-virtual `CalibrationBoard` + free `writePrintablePng()` + static helpers (`pointsToMat`, `pixelsPerMmFromDpi`, `dpiFromPixelsPerMm`). `writePrintableImage` is non-virtual (NVI), so every subclass gets a working printer for free.
  - `fanuc_irvision_board.{h,cpp}` — concrete `FanucIRvisionBoard` with the iRvision `Params`, presets, role enum, and all detection logic.
  - `calibration_board_factory.{h,cpp}` — `CalibrationBoardFactory` with `create(type)`, `createFanucIRvision(params)`, `createFromPreset(name)`, `availablePresets()`.
- Tests refactored: existing class renamed `TestCalibrationBoard` → `TestFanucIRvisionBoard`; added `TestCalibrationBoardFactory` (5 cases including polymorphic-only usage through a base pointer).
- Preset name strings adopted dash + decimal form to match the physical iRVision part numbers: `iRvision-5mm`, `iRvision-11.5mm`, `iRvision-15mm`, `iRvision-22.5mm`, `iRvision-30mm`.

### Round 10 — docs refresh
- Both `calibration_module.md` and `calibration_module.html` rewritten to cover the new architecture: file layout, ASCII architecture diagram, base/concrete split, factory section with preset table, polymorphic example, "extending the module" recipe.

### Round 11 — this file
- Session log committed for future archaeology.

---

## 4. Key design decisions worth remembering

| Decision | Rationale | Implication |
|---|---|---|
| Planar-only `Calibrator` (homography) | User confirmed 2D pick-and-place only | No `cv::calibrateCamera` / intrinsic-extrinsic split anywhere |
| 4 corner targets for calibration, **not** the centre/coord markers | Corner targets span the full board → well-conditioned homography. Coord+centre cluster at the middle. | Detection must surface the 4 corner positions reliably; everything else is informational. |
| Orientation from 4 large markers + geometry only | Area-based "small/large" split needs a `pxPerMm` estimate, which is brittle. Geometry (colinear triple + off-line marker) is scale- and rotation-free. | Detection algorithm no longer touches the inner white hole except for visualisation; tolerates aggressive downsampling. |
| Odd `rows`, `cols`; minimum 5 × 7 | Forces a unique centre cell; reserves room for `(rmid, cmid+2)` xTip and `(rmid−1, cmid)` yTip without colliding with the 4-corner/edge-midpoint target ring positions. | Validation in `Params::isValid()` enforces this; constructor throws `std::invalid_argument`. |
| White inner = tool-tip target | When the operator manually centres the robot probe on a ring, the white interior is the only feature visible at close range. | The 8 corner+edge targets and the centre ring all carry a tiny white circle (default 1.0 mm); coord dots stay solid because they are orientation-only. |
| `writePrintablePng` patches pHYs manually | OpenCV's `imwrite` doesn't expose PNG DPI metadata; pulling in libpng is heavy. | The module stays single-dependency (OpenCV); ~50 lines of CRC32+chunk-insertion code. |
| NVI `writePrintableImage` on the base | Every future subclass automatically gets a printable PNG once it implements `generateImage`. | Subclasses cannot accidentally diverge on the file format. |
| Subclass-specific accessors (`targetIndices`, `params`, …) live **outside** the base interface | Other board families won't have the same role structure (e.g. a ChArUco board has no "coord dot"). | Code that needs those accessors must `dynamic_cast<FanucIRvisionBoard*>` or construct the subclass directly. |
| Factory returns `unique_ptr<CalibrationBoard>` (base) | Callers can stay board-agnostic and configuration files can refer to boards by string. | Type-specific access requires a `dynamic_cast`; the demo `main.cpp` shows the pattern. |

---

## 5. Known limitations / open watch-list

- **Orientation ambiguity above ~45° rotation**: detection picks the corner with min `(x+y)` as TL by convention. If a real camera/board setup rotates the board by more than ~45°, the `[TL, TR, BR, BL]` labelling will reorder. In practice the operator records robot XY in the order the detector returns, so this is fine — but document it for new users.
- **Inner-white detection sensitivity**: at very small inner diameters (e.g. 0.5 mm @ 10 px/mm = 5 px), the threshold + contour pass may miss holes on noisy real images. Today's `detect()` no longer depends on hole detection for correctness — but the debug overlay's role-colouring won't reflect rings if the hole is missed.
- **Heuristic-free pxPerMm**: `detect()` infers nothing about pixel scale until after blob extraction; it relies on the 4 largest blobs forming a recognisable centre+coord geometry. If the board is partially occluded such that one coord dot is missing, detection fails outright. (Acceptable for calibration use case — the operator can recapture.)
- **`writePrintablePng` is PNG-only**: TIFF, BMP, JPEG paths would need their own DPI metadata patches. Out of scope.
- **Preset values for `iRvisionPattern5mm` / `15mm` / `22.5mm` / `30mm`** were chosen by the user to mirror physical Fanuc parts; they have not yet been verified against actual ink-printed boards. Worth re-checking once a real board is in hand.
- **`identifyOrientationMarkers4` sanity check**: requires that `xMid` lies within 25 % of the midpoint between `centre` and `xTip`. Real-world distortion approaching this threshold would cause detection to fail; if that happens, loosen the threshold rather than reach for image-rectification.

---

## 6. Test inventory at session end

`tests/tst_main.cpp` contains:

**TestFanucIRvisionBoard** (10 cases)
1. `params_evenRowsOrCols_throws`
2. `params_validOdd_constructsOk`
3. `generateImage_hasExpectedSize`
4. `objectPoints_countAndSpacing`
5. `markerIndices_returnsFourOrientationMarkers`
6. `targetIndices_returnsEightTargets`
7. `coordIndices_returnsThreeCoordDots`
8. `specialCells_doNotOverlap`
9. `cornerObjectPoints_areBoardCorners`
10. `detect_onGeneratedImage_returnsAllDots`
11. `detect_returnsFourCornersInImageOrder`
12. `detect_orientationOrder_isRowMajor`
13. `writePrintableImage_embedsPhysChunk`

**TestCalibrationBoardFactory** (5 cases)
1. `create_FanucIRvision_returnsConcrete`
2. `createFromPreset_knownName_returnsBoard`
3. `createFromPreset_unknownName_returnsNull`
4. `availablePresets_includesAllIRvisionEntries`
5. `polymorphicUsage_throughBasePointer`

**TestCalibrator** (5 cases)
1. `calibrate_lessThan4Points_returnsFalse`
2. `calibrate_perfectAffine_lowError`
3. `calibrate_fromFourCornersOnly`
4. `roundTrip_imageToRobotToImage`
5. `saveLoad_homographyPreserved`

All passing as of session end.

---

## 7. Pointers for the next session

- If you need to add a new board family, start at section 13 of [`calibration_module.md`](calibration_module.md) — three-step recipe.
- If detection regresses on a real Fanuc board image, the first place to look is `identifyOrientationMarkers4` in `fanuc_irvision_board.cpp`: print the 4 candidate marker positions and their pairwise distances; the geometry sanity-check (`midErr / xSpan > 0.25`) is the most common failure point.
- If you want to honour a configurable pxPerMm hint from the caller (e.g. when the camera intrinsics are known), the natural place is an optional parameter on `CalibrationBoard::detect`; today the detector deduces it from the 4 largest blobs and is intentionally hint-free.
- `description.md` at the project root is the running spec the user maintains; it accumulates "Unit tests output" / "Output run" / "Confirm points" / "Grid pattern format" / "Calibration board type develop" sections as feedback. Keep that file fresh — every round of feedback we got in this session was via that file.
