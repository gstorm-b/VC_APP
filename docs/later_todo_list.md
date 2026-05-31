# Later TODO list

Outstanding items that were flagged but intentionally deferred to keep PRs
focused. Each entry records WHAT, WHERE, WHY-deferred, and a rough hint on
how to pick it up.

---

## 1. `SignalsMapWidget::checkEmpty()` — caller not wired

**What.** `SignalsMapWidget::checkEmpty()` is destructive: it purges orphan
(warning-flagged) tags to `""` and returns the list of `internalName`s now
empty. Currently nothing in the codebase calls it.

**Where.** `src/widgets/signals_map_widget.{h,cpp}`, called from a not-yet-
chosen point in `src/form/task/localization_setting_widget.cpp`.

**Why deferred.** Caller policy is a UX decision: validate on Save? On Apply?
On commission start? Calling it eagerly inside `loadConfigToTask()` would
surface a confirm-style dialog on every field change, which is the wrong
moment. Owner needs an explicit validate trigger first.

**How to pick up.** Decide validate trigger (suggest: project Save and/or
commission start). Wire it to call `ui->listView_signals_map->checkEmpty()`
and surface the returned `QStringList` to the user (block save with a
dialog listing the missing mappings, or just log + allow).

---

## 2. Shared QSS design tokens for themed widgets

**What.** Three QSS files now hand-roll the same colour palette by hex
literal (`#2b8ce8`, `rgba(43, 140, 232, 28)`, etc.):
- `resrc/styles/add_device_wizard_{dark,light}.qss`
- `resrc/styles/camera_mapping_widget_{dark,light}.qss`

Any future themed widget added by following rule 15.1 will copy the same
constants again.

**Why deferred.** Refactoring all three files in one go widens the change
beyond the immediate task. Qt doesn't natively support QSS variables, so
the refactor needs a deliberate strategy (a small `.qss` snippet that
gets `@`-included by a generator, or a `resrc/styles/_tokens.qss` plus a
build-time concat step, or just a documented palette file).

**How to pick up.** Pick the cheapest strategy (probably: a single
`design_tokens.md` doc listing every token and which `.qss` files use it,
so renames stay tractable until we have enough widgets to justify a
generator).

**Update (2026-05-30).** The canonical token table is now **fully finalized**
in [ui_design_rules.md](ui_design_rules.md) §5, covering all six groups:

- §5.1 Background (bg.*) — 5 tokens
- §5.2 Border (border.*) — 4 tokens
- §5.3 Text (text.*) — 5 tokens
- §5.4 Accent + Selection (accent.*, selection.*) — 6 tokens
- §5.5 State + State surface (state.*) — 8 tokens  ← **confirmed, no longer proposed**
- §5.6 Panel accent (panel.accent.*) — 4 tokens

Active theme: **Hybrid — Graphite Vision background + Navy Ops Orange accent**.

What remains deferred: (a) the build-time mechanism (generator / `@`-include /
concat step) so `.qss` files stop carrying literal hex, and (b) the mechanical
migration sweep of existing `.qss` files — both tracked in #23.

---

## 3. `CalibrationBoardDialog` — preset-only selection

**What.** The dialog at `src/widgets/calibration/calibration_board_dialog.{h,cpp}`
only lets the user choose a named preset from
`CalibrationBoardFactory::availablePresets()`. There is no way to author
a custom `FanucIRvisionBoard::Params` (custom rows/cols/spacing/margins).

**Why deferred.** Current shop-floor flow uses one of the five iRVision
presets verbatim. Adding a full Params editor (and the validation around
`FanucIRvisionBoard::Params::isValid`) before there is demand would be
premature.

**How to pick up.** Add a second tab / collapsible group in the dialog
that exposes raw `Params` fields. On accept, build via
`CalibrationBoardFactory::createFanucIRvision(params)` instead of
`createFromPreset(name)`. Persistence needs a new key beyond
`DEVICE_JSK_CALIB_BOARD_PRESET` — probably the board's `toJson()` blob.

---

## 4. `EditableComboWidget::eventFilter` — combobox-is-popup-only design

**What.** `EditableComboWidget` keeps its `QComboBox` page hidden even
when it is the current widget: the `FocusIn` handler immediately switches
back to the label. The popup is the only visual the user ever sees.

**Status.** Intentional. Documented inline now as
`// Combobox is purely a popup host — its visual rep stays hidden.`

**Why noted here anyway.** A future contributor may flag it as a bug and
"fix" it. If we ever want the combobox to actually render inline (e.g.
for accessibility / keyboard-only navigation), the fix is to remove the
`FocusIn` branch and tune the stacked-widget transitions. Not needed
today.

---

## 5. `getCurrentMapping()` does not detect duplicate camera ids across rows

**What.** `CameraMappingWidget::provideNameOptions()` filters out cameras
already used by other rows when *that* row is being edited, so a duplicate
selection cannot happen via the UI. But if a caller injects a mapping
through `setCurrentMapping()` that already has two rows pointing to the
same id (corrupt save file, manual edit), the widget renders both rows
without warning.

**Why deferred.** Not observed in practice and would require either a
load-time sanity pass or a per-row warning style. Pick one once a real
corruption shows up.

**How to pick up.** In `setCurrentMapping()`, after the rebuild, walk the
rows and apply a warning style (similar to `SignalsMapWidget::applyWarning`)
to any row whose id is also used by an earlier row.

---

## 6. `RobotDevice` — vendor API surface not yet defined

**What.** `src/device/robot/robot_device.h` currently only declares the
family-level dispatch (`RobotType` + `robotType()` pure virtual) plus the
mandatory `IDevice` overrides. There is no shared abstraction for motion,
teach-pendant, IO, frame transforms, etc. The two concrete subclasses
(`KawasakiRobotDevice`, `NachiRobotDevice`) return stubs for every
required override (connect/disconnect/isConnected return false; bits and
words lists are empty; `pushRequest` returns false).

**Why deferred.** Per Rule 12.5, the abstract is promoted only when a
real implementation reveals what the shared surface should look like.
Designing motion APIs against zero vendor experience would lock us into
the wrong abstraction.

**How to pick up.** When the first vendor integration starts (likely
Kawasaki), implement that vendor's real protocol in
`kawasaki_robot_device.cpp` first. Once a second vendor (Nachi or Huayan)
is being implemented, extract the shared methods into `RobotDevice` as
pure virtuals and migrate the existing impl. Don't try to design the
abstraction before the second vendor exists.

---

## 7. `AddDeviceWizard` — no Robot card

**What.** The wizard at `src/form/add_device_wizard.{h,cpp,ui}` currently
has cards for Camera, MC and VisionOutput only. Robot devices cannot be
created through the UI, even though `DeviceFactory::createRobotDevice`
and `DeviceManager::getSubDeviceTypeList(Robot)` are both wired.

**Why deferred.** The Robot framework was added without UI in scope (user
asked for the device-side framework only). Adding a card requires:
designing the icon + colour token (analog to `deviceColor="camera"`),
authoring matching dark/light QSS, registering the card in `initCards()`,
and a sub-type combobox feeding from `getSubDeviceTypeList(Robot)`.

**How to pick up.** Mirror the MC card. Pick a colour key (e.g.
`deviceColor="robot"` with an orange-ish accent distinct from
visionOutput). Add the card frame + sub-type combo to `add_device_wizard.ui`,
register it via `bind()` in `initCards()`, and append a matching
`QPushButton#adwAddBtn[deviceColor="robot"]` block to both QSS files.

---

## 8. `RobotRunner` — no runtime wiring

**What.** Camera, MC and VisionOutput each have a `*Runner` class in
`src/runtime/` that mediates between the GUI thread and the device's
own thread, and `TaskRunner::registerDevice` knows how to spin one up.
There is no `RobotRunner` and no dispatch case for `DeviceType::Robot`
in the task runner.

**Why deferred.** Without a real vendor protocol there is nothing to
route across thread boundaries. The Robot stubs all return synchronously.

**How to pick up.** When the first vendor implementation needs blocking
I/O (TCP socket to teach pendant, serial line, etc.), create
`src/runtime/robot_runner.h` modelled on `camera_runner.h`. Add a
dispatch case in `TaskRunner::registerDevice` and any task that consumes
a robot will need an `assignedDevicesOfType(Robot)` helper similar to
how `localization_setting_widget.cpp` currently uses camera/comm devices.

---

## 9. `VisionSerial` — sub-type declared, no implementation

**What.** `VisionOutputType::VisionSerial` is registered in
`src/device/output_device/vision_output_config.h` (the enum) and in the
`VisionOutputTypeToString/FromString` helpers in
`vision_output_device.h`, but:
- `DeviceFactory::createVisionOutputDevice` returns nullptr for that case.
- `AddDeviceWizard::buildDeviceJson` skips it (no concrete config to build).
- `DeviceManager::subDeviceTypeLists[VisionOutput]` only contains
  `VisionTCPIP`, so the wizard combobox never offers it to the user.

**Why deferred.** Same as Robot vendors — no real serial-protocol spec
yet. Designing the abstraction with zero impl would lock the wrong shape.

**How to pick up.** Add `vision_serial_config.h` + `vision_serial_device.{h,cpp}`
mirroring the TCP pair, wire a `createVisionSerial` factory branch,
register `VisionTypeToString(VisionSerial)` in
`DeviceManager::subDeviceTypeLists`, and add a wizard branch in
`buildDeviceJson`. The new widget (see #10) will also be needed.

---

## 10. `VisionOutputDeviceWidget` is TCP-only

**Status.** Completed in Phase 1. The TCP-specific widget is now
`VisionTcpipDeviceWidget`, and `DeviceWidgetFactory` owns subtype dispatch.

**Remaining future work.** When `VisionSerialDevice` lands, create
`VisionSerialDeviceWidget` and add a `VisionSerial` branch to
`DeviceWidgetFactory`.

---

## 11. `VisionOutputRunner` may need transport-specific signals

**What.** `src/runtime/vision_output_runner.h` currently exposes only
`requestConnect/Disconnect` and forwards `connectStatusChanged` /
`errorOccurred` — all transport-agnostic surface from `IDevice`. It
holds a `VisionOutputDevice*` (the abstract base), so it works for any
concrete sub-type.

**Status.** No issue today; works for TCP and will work for serial.

**Why noted here anyway.** If a sub-type ever needs to surface
transport-specific signals through the runner (e.g. serial framing
errors, TCP heartbeat-lost statistics), the runner has to be split or
templatised. Until then, leave alone.

---

## 13. Widget `static_cast` to concrete device is unsafe

**What.** Both `BaslerCameraWidget::initCameraWiget` and
`VisionTcpipDeviceWidget::initWidget` use `static_cast` to downcast
from `IDevice*` to the concrete device type
(`BaslerGigECamera*` / `VisionTcpipDevice*`). If a future caller passes
in a device of a different sub-type (e.g. a Realsense camera reaches
the Basler widget by mistake) the cast silently produces a wrong-typed
pointer and the next member access is UB.

**Why deferred.** No misroute has been observed in practice. Both widgets
are constructed in `localization_task_widget.cpp` from a switch on
`device->deviceType()` + sub-type — the typing is enforced at the call
site, just not at the widget boundary.

**How to pick up.** Replace `static_cast` with `qobject_cast` plus a
nullptr check that logs `LOG_DEV_ERR` and disables the widget surface
(no crash, just no-op). Touchpoints:
- `src/form/camera/basler_camera_widget.cpp` ~ line 225.
- `src/form/vision_output/vision_tcpip_device_widget.cpp` ~ line 139.

---

## 14. `cbxVisionType` shown even with a single sub-type

**What.** `AddDeviceWizard` `pgVisionOutput` page renders
`cbxVisionType` combobox unconditionally, even though
`DeviceManager::subDeviceTypeLists[VisionOutput]` only contains
`VisionTCPIP` right now. Same pattern as `cbxCameraType` (single entry
`BaslerGigE`).

**Status.** Intentional consistency with Camera — leaves room for
forward-compat when the second sub-type is added without UI churn.

**Why noted.** A reviewer may flag it as "useless control"; this entry
documents that the control is kept on purpose, and should be made
conditional only if the project decides single-sub-type cards should
hide their combo (would also apply to `cbxCameraType`).

---

## 15. `subDeviceTypeLists[PLC]` carries McFrame strings, not `PlcType`

**What.** After the PLC sub-type promotion, `DeviceManager::subDeviceTypeLists`
maps `DeviceType::PLC` to the Mitsubishi MC frame-type list
(`["Frame_3E"]`), not to the family-level `PlcType` list (which would be
`["MitsubishiMc"]`). The AddDeviceWizard relies on this to populate
`cbxMcFrameType`, which is a protocol-level setting specific to
Mitsubishi MC — wrong abstraction level.

**Where.** `src/device/device_manager.cpp` near the constructor's
`subDeviceTypeLists.insert(DeviceType::PLC, mc_type_strlist)` line
(commented inline already).

**Why deferred.** No second PLC vendor exists yet; the abuse is
invisible to users until Omron FINS or Siemens S7 arrives and needs its
own protocol-level options (different frame types, different addressing).

**How to pick up.** When vendor #2 lands:
1. Change `subDeviceTypeLists[PLC]` to actually carry `PlcType` strings.
2. Introduce a parallel per-vendor protocol-option source (e.g.
   `DeviceManager::getProtocolOptions(PlcType)` returning a struct of
   per-vendor combo contents).
3. AddDeviceWizard reads `subDeviceTypeLists[PLC]` to fill a new
   `cbxPlcType` combo; the existing `cbxMcFrameType` / `cbxMcCode`
   become conditional on `cbxPlcType == MitsubishiMc`.

---

## 20. AddDeviceWizard stack page still named `pgMc`

**What.** The PLC card was renamed (`adwCard_mc → adwCard_plc`,
`deviceColor="mc" → "plc"`), but the underlying stack page inside
`pgMcLayout` / the page object `pgMc` and its children
(`cbxMcFrameType`, `cbxMcCode`, `lblMcFrame`, `lblMcCode`) still use
the `Mc` prefix.

**Where.** `src/form/add_device_wizard.ui` around the visionOutput-page
neighbourhood.

**Status.** Cosmetic only. The widgets belong to the Mitsubishi MC
sub-type's protocol options, so the `Mc` prefix is technically correct
at that level — the inconsistency is that the outer card was promoted
to the family name while the inner controls stayed protocol-named.

**Why deferred.** Pure cosmetic; renaming requires editing the .ui
plus every `ui->cbxMcFrameType` access in `add_device_wizard.cpp`
without any functional gain until #15 actually splits the wizard
into a 2-level (PlcType → protocol options) flow.

**How to pick up.** Rename together with the work in #15 — when the
inner controls become conditional on a `cbxPlcType` selection, give
them the per-vendor names at the same time
(`pgMitsubishi`, `cbxMitsubishiFrameType`, …).

---

## 21. `TaskLocalization::matchingRunner` has no explicit teardown

**What.** `TaskLocalization` creates `matchingRunner = new QThread()` in its
constructor and starts it immediately, but the class does not declare a
destructor that quits, waits for, and deletes the thread.

**Where.** `src/model/task_localization.cpp` constructor and
`src/model/task_localization.h` member `QThread *matchingRunner`.

**Why deferred.** It was noticed while adding Phase 0 architecture contract
tests. Fixing runtime thread ownership is production behavior and should be a
focused lifecycle change, not mixed into test scaffolding.

**How to pick up.** Add a `TaskLocalization` destructor that stops matching
commission work safely, calls `matchingRunner->quit()`, waits with a bounded
timeout, and deletes the thread. Verify task factory tests and any commission
matching path still shut down cleanly.

---

## 22. Code review findings — qt-cpp-review (2026-05-30)

Batch of findings from a structured `qt-cpp-review` over the outstanding
device-binding / vision-widget / task-localization changes. Logged here per
Rule 11.2 (flag, don't silently fix). Items already tracked elsewhere are NOT
repeated; for example, the `matchingRunner` teardown is #21.

**Tech-lead note.** 22.1–22.2 are correctness bugs, not genuine "defer to
keep the PR focused" items — they are parked here only because the owner asked
to flag-not-fix this round. Items fixed by Phase 2 have been removed from this
batch. The remaining items should be scheduled before the next
commission run, ahead of the cleanup items lower down.

### Critical — schedule before next commission run

**22.1 — `std::shared_ptr::reset(raw)` creates a second owning control block.**
- Where. [task_localization.cpp:261](../src/model/task_localization.cpp) (and
  :266, :267, :320). `m_nextConnectCamera.reset(camera)` / `m_selectedCamera.reset(camera)`
  receive a raw `CameraDevice*` from `qobject_cast<CameraDevice*>(device.get())`,
  where `device` is a `std::shared_ptr<IDevice>` owned by `DeviceManager`.
- Why it matters. `reset(raw)` builds a brand-new control block over an object
  already owned elsewhere → double-free / dangling when either owner drops.
- How to fix. Keep the original `shared_ptr<IDevice>` and use
  `std::static_pointer_cast`/`dynamic_pointer_cast`, assigning with `=` so all
  owners share one control block. Never `reset()` with a raw pointer pulled from
  another `shared_ptr`.

**22.2 — Cross-thread queued matching signals carry unregistered metatypes.**
- Where. [task_localization.h:89,95](../src/model/task_localization.h) +
  [task_localization.cpp:381-418](../src/model/task_localization.cpp).
  `startCommissionMatchingRequest(std::shared_ptr<mtc::MatchGroup>, cv::Mat)` and
  `commissionMatchingFinished(mtc::MatchResult)` cross thread boundaries (worker
  on `matchingRunner`) → `Qt::QueuedConnection`.
- Why it matters. Queued connections marshal each argument through the metatype
  system. `mtc::MatchResult`, `cv::Mat`, and `std::shared_ptr<mtc::MatchGroup>`
  are not registered, so Qt logs "Cannot queue arguments of type 'cv::Mat'" and
  silently drops the call — commission matching never runs.
- How to fix. `Q_DECLARE_METATYPE` for the non-Qt types and `qRegisterMetaType`
  once at startup for all three.

### High — real bugs, scoped fixes

**22.3 — `removeWidget()` during forward index iteration skips pages.**
- Where. [localization_task_widget.cpp:470](../src/form/task/localization_task_widget.cpp),
  `onTaskDevicesChanged()`. Loop iterates `content_stack` by ascending index and
  removes inside the loop; `removeWidget` shifts later indices down while `idx`
  still increments, so a page right after a removed one is never visited. Two
  consecutive removed device pages leave a stale `IDeviceWidget` behind.
- How to fix. Collect widgets to remove in one pass, delete in a second; or
  iterate `idx = count()-1 .. 0`.

**22.4 — `m_devicePages` cache evicted for still-assigned devices.**
- Where. [localization_task_widget.cpp:479](../src/form/task/localization_task_widget.cpp).
  `removePropertyBrowserWidget(...)` and `m_devicePages.remove(deviceId)` sit
  outside the `if (!deviceIds.contains(deviceId))` guard, so they run for every
  page found — including live ones. Cache and stack desync; a later
  `showDeviceConfigPage` misses the cache and builds a duplicate page, orphaning
  the original.
- How to fix. Move both calls inside the `if (!deviceIds.contains(deviceId))`
  block.

**22.5 — `IDeviceWidget` constructor drops its `parent` argument.**
- Where. [device_widget.h:12](../src/form/device_widget.h). `IDeviceWidget(QWidget *parent = nullptr) {}`
  has an empty init list, so `QWidget` is default-constructed and `parent` is
  discarded. Subclasses forward `parent` expecting parent-child ownership.
- Note. Base class is outside the reviewed changeset but every device widget
  depends on it.
- How to fix. `IDeviceWidget(QWidget *parent = nullptr) : QWidget(parent) {}`.

**22.6 — Shared `MatchGroup` read on worker thread while GUI can mutate it.**
- Where. [task_localization.cpp:382-404](../src/model/task_localization.cpp). The
  worker lambda iterates `group->patterns()` / reads `config()` on the matching
  thread, holding the same `shared_ptr<MatchGroup>` that `PatternGroupManager`
  (GUI thread) keeps mutating (`addPattern`/`removePattern`/`setPatternImage`).
  `MatchGroup` is non-QObject with no locking (design_rules §4.2) — the container
  read races the GUI-thread append/erase.
- How to fix. Snapshot a deep copy of the needed config + cloned train images on
  the GUI thread before emitting, and hand only the copy to the worker; or
  serialize all `MatchGroup` access with a mutex; or block pattern editing while a
  commission match is in flight.

### Medium — hardening / robustness

**22.8 — `fromJson` performs no schema/version validation.**
- Where. [task_localization_config.h:89](../src/model/task_localization_config.h),
  [task_localization.cpp:116](../src/model/task_localization.cpp). Only gate is
  `obj.empty()`; fields read with defaults, so a future/foreign document is
  silently accepted with partial-default state. `toJson()` writes no version key.
- How to fix. Add a `version` int to `toJson()` and validate/migrate it in
  `fromJson()`; treat missing as the legacy baseline.

**22.9 — No range validation on imported binding data.**
- Where. [task_device_binding.h:65](../src/model/task_device_binding.h).
  `cameraNumber = obj["cameraNumber"].toInt(0)` accepts any int including
  negatives, while the task enforces 1..16 elsewhere (`limit_num_camera`). A
  malformed/hostile file can inject out-of-range numbers.
- How to fix. Validate `cameraNumber` against the legal range (and cap device-id
  string length) in `fromJson`; drop/clamp invalid entries via the existing
  `bool`-return convention.

**22.10 — `IDeviceWidget` polymorphic base lacks virtual dtor / `Q_DISABLE_COPY_MOVE`.**
- Where. [device_widget.h:8](../src/form/device_widget.h). Declares pure virtuals
  but no explicit virtual destructor and no `Q_DISABLE_COPY_MOVE`.
- How to fix. Add `Q_DISABLE_COPY_MOVE(IDeviceWidget)` and
  `~IDeviceWidget() override = default;`. (Base class — coordinate with 22.5.)

**22.11 — `m_output_device` may be dereferenced uninitialized.**
- Where. [vision_tcpip_device_widget.h:54](../src/form/vision_output/vision_tcpip_device_widget.h).
  No in-class initializer; assigned only inside `if (m_device)` in `initWidget()`,
  but `saveConfig()` derefs unconditionally. The factory currently guards device
  null, so the bad path is not reachable today — but the invariant is implicit.
- How to fix. Initialize `m_output_device{nullptr}` and null-check before deref,
  or assert the device invariant at construction. Related to #13 (unsafe casts in
  the same widget).

**22.12 — `taskRunner()` dereferenced without null check in runner helpers.**
- Where. [task_localization.cpp:85-95](../src/model/task_localization.cpp).
  `cameraRunner()` / `plcRunner()` call `taskRunner()->runnerFor(...)` with no
  guard, while the task widget treats `taskRunner()` as possibly null
  ([localization_task_widget.cpp:781](../src/form/task/localization_task_widget.cpp)).
- How to fix. Add a null guard returning nullptr for consistency.

**22.13 — Reconnect `SingleShotConnection` re-arms against the wrong camera.**
- Where. [task_localization.cpp:325](../src/model/task_localization.cpp).
  `waitReconnectCameraHandle` re-connects to `m_selectedCamera` for non-terminal
  statuses, but the device being awaited is `m_nextConnectCamera`. Combined with
  22.1, it can wire the wait onto a soon-to-dangle sender.
- How to fix. Confirm which device the wait targets, disconnect prior connections
  before re-arming, and avoid re-arming on a sender whose `shared_ptr` may reset.

### Low — cleanup / quality

**22.14 — Dead member variables.**
- Where. [task_localization.h:121-132](../src/model/task_localization.h).
  `m_currentCamNumber`, `m_curentPatternNumber` (also a typo), `m_lastMatchResult`,
  and `m_lastVisionOutput` have no read/write sites.
- How to fix. Remove them, or gate behind the feature when it lands; fix the typo
  if kept.

**22.16 — `switch` over `ConnectStatus` uses `default:`, hiding new cases.**
- Where. [task_localization.cpp:347](../src/model/task_localization.cpp),
  [vision_tcpip_device_widget.cpp:224](../src/form/vision_output/vision_tcpip_device_widget.cpp).
- How to fix. Enumerate every `ConnectStatus` value explicitly (no-ops with
  `break;`) and drop `default:` so `-Wswitch` flags additions.

**22.17 — Duplicated meta-property lookup/dispatch logic.**
- Where. [localization_setting_widget.cpp:62](../src/form/task/localization_setting_widget.cpp)
  (`readConfigField`/`writeConfigField`) and
  [vision_tcpip_device_widget.cpp:166](../src/form/vision_output/vision_tcpip_device_widget.cpp)
  re-implement the "indexOfProperty → write/readOnGadget" pattern and class-info
  `_name` resolution independently.
- How to fix. Extract a shared `gadget_meta` helper and call from both.

**22.18 — Public `const` data members used as limits.**
- Where. [task_localization.h:105-107](../src/model/task_localization.h).
  `limit_comm_device`, `limit_vision_output_device`, `limit_num_camera` are
  public non-static snake_case const members.
- How to fix. Make them `static constexpr int` with a consistent name scheme
  (e.g. `kLimitNumCamera`).

**22.19 — Fixed page-index constants assume a click order that isn't enforced.**
- Where. [localization_task_widget.cpp:616](../src/form/task/localization_task_widget.cpp).
  `kDashboardPage/kSettingsPage/kPatternsPage` are used both as `insertWidget`
  positions and `setCurrentIndex` targets, but pages are created lazily in user
  order, mixed with `setCurrentWidget(...)` navigation elsewhere.
- How to fix. Navigate by widget pointer consistently
  (`setCurrentWidget(m_settingPage)`), or build all fixed pages once up front.

**22.20 — `cameraNumberMap()` rebuilds a `QMap` by linear scan on a hot path.**
- Where. [task_device_binding.h:103](../src/model/task_device_binding.h).
  `cameraDeviceId()` is called from `setCameraNumber()` on every camera-switch
  signal; each call scans the `QList` and allocates a fresh map. Fine at ≤16
  cameras; revisit only if binding counts grow.
- How to fix. Cache a `QHash<int,QString>` invalidated on `setCameraNumberMap()`
  if profiling shows it matters.

**22.21 — `saveConfig()` ignores the persistence outcome.**
- Where. [vision_tcpip_device_widget.cpp:208](../src/form/vision_output/vision_tcpip_device_widget.cpp).
  `setVisionTcpipConfig(m_config)` result is discarded; called after every edit
  with no success/failure feedback.
- How to fix. If the setter can fail or persists to disk, return a status and
  surface failures (log + visual), matching the `changeDeviceName` pattern.

**22.22 — Rename failure is reverted but not surfaced to the user.**
- Where. [vision_tcpip_device_widget.cpp:190](../src/form/vision_output/vision_tcpip_device_widget.cpp).
  When `changeDeviceName` returns false the field is reset with no user-visible
  reason. Same silent idiom as the basler / mc_protocol widgets — uniform but
  uniformly silent on a user-facing failure.
- How to fix. On false, add a user-level log/toast explaining the rejection in
  addition to reverting the field (applies to the sibling widgets too).

---

## 23. UI conformance migration to ui_design_rules.md

**Status (2026-05-30): Partially complete — theme-reload contract fully
implemented; hex-token migration still pending.**

**Completed in this pass:**
- `IDeviceWidget` and `ITaskWidget` now both provide `virtual reloadStyleSheet()`
  and `setupThemeReload(darkPath, lightPath)`. Subclasses call `setupThemeReload`
  once from their constructor; the base handles the initial load and the
  `ThemeManager::themeChanged` subscription.
- Fixed `IDeviceWidget` constructor bug: `parent` was not forwarded to `QWidget`.
- All four subclasses that had per-form QSS now use `setupThemeReload` instead of
  duplicating the reload/connect boilerplate:
  `MitsubishiMcDeviceWidget`, `LocalizationTaskWidget`, `LocalizationPatternsWidget`,
  `VisionTcpipDeviceWidget`.
- Created missing QSS pairs that were referenced in code but absent or not
  registered: `localization_patterns_widget_{dark,light}.qss` (new files),
  `localization_task_widget_{dark,light}.qss` (existed on disk, now registered).
- Fixed `VisionTcpipDeviceWidget::updateConnectionVisual()`: removed three
  inline `setStyleSheet()` calls; connection state now driven by
  `setProperty("connectionState", ...)` + repolish, styled in the new
  per-form QSS pair via attribute selectors (ui_design_rules §3.6, §4.5).
- Removed unused `#include "form/pattern/pattern_theme.h"` from
  `vision_tcpip_device_widget.cpp`.
- All six new/fixed QSS pairs registered in `resrc.qrc`.

**Still open (what remains of the original #23):**
- **Hardcoded hex to tokens.** Per-form sheets
  (`add_device_wizard_{dark,light}.qss`, `camera_mapping_widget_{dark,light}.qss`,
  `signals_monitor_widget_{dark,light}.qss`) and the global
  `dark.qss`/`light.qss` still carry literal hex values. Reconcile them onto
  the §5 token table (this is the migration half of #2). Blocked on deciding
  the build-time token mechanism (concat / `@`-include / generator).
- **Accent-tinted panels.** Any remaining blue-tinted literals (`#2a3a52`,
  `#1a2540`, `#111f30`) in old QSS files must be replaced with the orange-warm
  `panel.accent.*` family defined in ui_design_rules.md §5.6.

**How to pick up remaining work.** Decide the token mechanism (#2), then sweep
each `.qss` replacing literal hex with §5 canonical values. Run the §9 review
checklist against each migrated file in both dark and light.

---

## 24. `svgIcon()` is not theme-aware — Rule 4.4 violation (project-wide)

**What.** Every icon in the project is set via `svgIcon()` (defined in
`windows_helper.h:85`), a thin wrapper that loads a single SVG path without
consulting `ThemeManager`. `ThemeManager::themedIcon()` was declared in
`src/utils/theme_manager.h:43` and implemented in `theme_manager.cpp:75` but
is **never called anywhere** in the codebase.

Affected call sites (representative, not exhaustive):
- `mainwindow.cpp` lines 117–139 — toolbar action icons
- `src/form/task/localization_task_widget.cpp` lines 208–448 — nav/breadcrumb icons
- `src/widgets/project_tree_widget.cpp` lines 132–341 — tree item icons
- `src/form/camera/basler_camera_widget.cpp` lines 211–412 — connect/trigger icons
- `src/form/add_device_wizard.cpp` line 74 — device card icons

**Why deferred.** The current SVG assets appear to be monochrome/outline style
that renders acceptably in both themes. Fixing Rule 4.4 requires either:
(a) auditing whether any icon actually needs a dark/light variant, or
(b) deciding to drop `themedIcon()` if all icons are theme-neutral (and updating
Rule 4.4 accordingly).

**How to pick up.**
1. Audit all icon assets under `resrc/` — identify which ones need per-theme
   variants (light-on-dark vs dark-on-light).
2. For icons that need variants: create `_dark.svg` siblings and replace
   `svgIcon(path)` calls with `ThemeManager::instance()->themedIcon(basePath)`.
3. For icons that are theme-neutral: document the exemption and consider
   removing `themedIcon()` if it is never needed, or keeping it for future use.
4. Update Rule 4.4 in `ui_design_rules.md` to reflect the actual convention.

---

## 25. `LocalizationDashboardWidget` backend wiring to the refactored `.ui`

**Status (2026-05-31): RESOLVED.** `localization_dashboard_widget.{h,cpp}` were
adapted to the refactored `.ui`. All pre-refactor `objectName` references were
migrated and the five behavioural changes below were implemented. The widget now
matches the operator-runtime mockup (`docs/task_localization_dashboard_mockup.html`)
and the implementation plan (`docs/task_localization_implementation_plan.md`).

**What was done:**
- objectName migration applied (`gv_match_view`, `wg_signal_monitor`,
  `lbl_val_vision_device`, `lbl_val_plc_device`, `lbl_val_camera`,
  `lbl_val_pattern_group`, `tbl_result`, `lbl_kpi_*_val`, `log_task_view`).
- `updateTaskStateLabel()` now drives the **Cycle** lamp from `TaskState`
  (Faulted→Error, RunningCycle/Recovering→Warning, Ready→Ok, else Off).
- New `applySignalToDashboard(name, value)` routes live signals to the
  Task/Camera/Pattern lamps and context value labels.
- New `setFaultState(active, code)` drives `frame_fault[active]` (setProperty +
  repolish, Rule 4.5), the fault value labels, and the Task lamp Error state;
  fault-code text uses `localizationFaultCodeName()`.
- `appendTaskLog()` now calls `log_task_view->appendEvent(TaskEvent)` with
  `severityToLevel()` mapping the runtime severity string → `TaskEventLevel`.
- Context labels write **value-only** (`lbl_val_*`); captions stay static in `.ui`.
- Both custom widgets (`StatusLamp`, `TaskEventLogWidget`) are promoted in the
  `.ui` and registered in `ncr_picking.pro`. Styling lives in global
  `dark.qss`/`light.qss` (no per-form pair, no `resrc.qrc` change).

**Residual dependency update (2026-05-31):** `TaskLocalization` now exposes the
`signalChanged(QString, QVariant)` path through `ITask`, and runtime output/input
updates are forwarded by `LocalizationRuntimeController`. Dashboard v1 is still
intentionally read-only, so monitor row writes remain disabled.

**Remaining verification (when the project builds end-to-end):** run the §9
review checklist in both dark and light; confirm lamps, fault panel, KPIs,
result table, and operator log update on a runtime cycle, and that the dashboard
exposes no manual write / trigger / start-stop controls (read-only v1).

---

## 26. Localization runtime production follow-ups after first implementation pass

**Status (2026-05-31): OPEN.** The first implementation pass builds and covers
the main contract shape, but `TaskLocalization` should not yet be considered
production-complete. The items below are follow-ups from the localization
runtime implementation.

**Remaining work.**
- Move the `LocalizationRuntimeController` object into `TaskRunner::m_runtimeThread`
  or explicitly document that the task object thread is the supported
  coordinator thread. Moving it requires queued task-to-controller invocations
  and adjusted QObject ownership.
- Move runtime matching off the controller call stack if cycle latency or UI
  responsiveness is a concern. The current pass runs matching synchronously
  after the camera frame arrives.
- Rework runtime active-camera switching so it is fully runner-based. The
  current `TaskLocalization::setCameraNumber()` path still contains direct
  `CameraDevice::deviceDisconnect()` / `deviceConnect()` calls and should be
  replaced by queued `CameraRunner` requests coordinated by
  `LocalizationRuntimeController`.
- Add focused runtime/controller tests for trigger edge behavior, held-trigger
  suppression, trigger reset, grab timeout fault `102`, VisionOutput send
  failure fault `201`, invalid pattern fault `400`, invalid calibration fault
  `401`, and lost device handling during `RunningCycle`.
- Add PLC write behavior tests for valid `M` bit tags, valid `D` word tags, and
  invalid tag rejection once a suitable fake PLC request sink exists.
- Run an operator UI verification pass against a real or simulated runtime
  cycle to confirm dashboard lamps, fault panel, KPIs, result table, and
  task-local log updates.

**Verification already done in the first pass.**
- `architecture_contract_test` built and ran with exit code `0`.
- Full Debug app build completed successfully.
