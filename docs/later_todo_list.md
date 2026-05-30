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

**What.** `src/form/vision_output/vision_output_device_widget.{h,cpp}`
holds `VisionTcpipDevice*` and `VisionTcpipDeviceCfg` directly. The
name `VisionOutputDeviceWidget` (and folder `form/vision_output/`)
suggests family-level scope, but the implementation is hard-bound to
the TCP sub-type and would not display a VisionSerial config.

**Why deferred.** Only one sub-type exists today; introducing a widget
dispatcher / abstract base before there is a second concrete widget
would be premature abstraction.

**How to pick up.** When `VisionSerialDevice` lands:
1. Rename the current widget to `VisionTcpipDeviceWidget` (file + class).
2. Create `VisionSerialDeviceWidget` analogous to it.
3. Decide where the dispatch lives — likely at the caller in
   `localization_task_widget.cpp:785` (`new VisionOutputDeviceWidget(...)`)
   needs to switch on the concrete sub-type from the device.

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
`VisionOutputDeviceWidget::initWidget` use `static_cast` to downcast
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
- `src/form/vision_output/vision_output_device_widget.cpp` ~ line 139.

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

## 18. `McProtocolDeviceWidget` is Mitsubishi-only

**What.** Mirror of #10 (`VisionOutputDeviceWidget` TCP-only). The
widget at `src/form/plc/mc_protocol_device_widget.{h,cpp}` holds a
`McProtocolDevice *` and `McProtocolConfig` directly, and its UI shows
Mitsubishi MC-specific fields (frame type, data code). When a second
vendor lands the family-level scope vs the concrete impl drift the
same way as VisionOutputDeviceWidget.

**Why deferred.** Single sub-type — no dispatcher needed yet.

**How to pick up.** When vendor #2 ships:
1. Rename `mc_protocol_device_widget.*` → e.g.
   `mitsubishi_mc_widget.*` (file + class).
2. Create `OmronFinsWidget` (or whichever vendor lands).
3. Dispatch at the call site in `localization_task_widget.cpp:778`
   on `plcType()`.

---

## 19. `TaskLocalization::onCommDeviceValueChanged` not yet wired

**What.** `TaskLocalization::onCommDeviceValueChanged(QMap<QString,QVariant>)`
is defined ([task_localization.cpp:279](../src/model/task_localization.cpp))
to translate PLC tag → logical signal name via `m_currentSignalsMap` and
re-emit as `signalChanged(name, value)`. The slot has no caller — no
`setupTask()` (or any other site) connects `plcRunner()->valueChanged`
to it. End-to-end signal path (`McProtocolDevice::valueChanged` →
`PlcRunner::valueChanged` → `TaskLocalization::onCommDeviceValueChanged`
→ `signalChanged`) is broken at the last hop.

**Where.** `src/model/task_localization.cpp::setupTask()` — should
register the connection once a PLC role is assigned. Also depends on
`m_currentSignalsMap` being populated from `TaskLocalizeConfig` (current
population path not yet traced — likely in `setTaskLocalizeConfig` or a
helper called from there).

**Why deferred.** This is the consumer end of the abstract PLC signal
path. The consumer that drives concrete shape decisions is the future
signal-monitor widget (see [docs/signal_map/](signal_map/)). Wiring
`onCommDeviceValueChanged` independently is functional but unverified
until that widget exercises it.

**How to pick up.** In `setupTask()`, once
`taskRunner()->hasRunner(m_plcDeviceId)` is confirmed, add:
```cpp
auto *plcRun = plcRunner(m_plcDeviceId);
connect(plcRun, &vc::runtime::PlcRunner::valueChanged,
        this,   &TaskLocalization::onCommDeviceValueChanged,
        Qt::UniqueConnection);
```
Then trace where `m_currentSignalsMap` is supposed to be filled from
`TaskLocalizeConfig` and add that path (it is logically the inverse of
the `SignalsMapWidget` schema → config write performed by
`LocalizationSettingWidget`).

**Status.** Signal *emission* and *runner forwarding* are now complete
(McProtocolDevice emits `pollingUpdate`/`valueChanged` via the
inherited `PlcDevice` signals; `PlcRunner` forwards both). Only the
task-side connect remains.

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
