# Design & Code Organization Rules — Session Compendium

This document captures the design rules, conventions, and code organization
patterns that emerged or were enforced across the session. Each rule lists the
**rule**, the **why**, and **where it was applied** so future contributors can
judge edge cases instead of blindly following the letter of the rule.

---

## 1. Property Browser — Custom Property Types

### 1.1 Composite property types use a `Mode` enum, not separate classes

**Rule.** When introducing a new composite property (e.g. PointXY / PointXYZ /
PointXYZDouble), expose it through a single `PropertyManager` class with a
`Mode` enum (`XY`, `XYZ`, `XYZDouble`, …) rather than one manager per shape.

**Why.** All variants share the same factory wiring, the same value-change
plumbing, and the same property-tree presentation. Splitting them into N
classes balloons the public surface and duplicates the sub-property creation
logic.

**Where applied.**
- [src/widgets/property_browser/custom_property_managers.h](src/widgets/property_browser/custom_property_managers.h) — `PointPropertyManager` (int) and `PointFPropertyManager` (double) each gate behaviour on `Mode`.

### 1.2 OpenCV interop is `__has_include`-gated

**Rule.** Overloads that depend on `<opencv2/core/types.hpp>` (cv::Point,
cv::Point2f, cv::Point3i, cv::Point3d) must be compiled-out when OpenCV is not
on the include path.

**Why.** The property browser library is intended to remain usable in
non-vision projects. Hard dependency on OpenCV breaks reuse.

**Where applied.**
- `custom_property_managers.h` — guarded blocks around cv::Point overloads.

### 1.3 Factories are registered once, at the browser-widget level

**Rule.** `QtSpinBoxFactory` / `QtDoubleSpinBoxFactory` are owned and connected
inside `PropertyBrowserWidget`, not inside the page that uses the property.

**Why.** Factories own editor lifetime; registering them in a page that gets
destroyed creates dangling editor widgets.

**Where applied.**
- [src/widgets/property_browser/property_browser_widget.cpp](src/widgets/property_browser/property_browser_widget.cpp) — single registration block.

---

## 2. Declarative Property Specs (`PropSpec<Config>`)

### 2.1 Property layout for a Config struct is declared as a `constexpr` table

**Rule.** Every `Config` struct that drives a property browser has a
file-scope `constexpr` array of `PropSpec<Config>` describing field name,
display label, group, units, and range. Code that builds the browser iterates
this table — it does not hard-code property creation.

**Why.** Adding a new field becomes a one-line table entry. The rebuild
function does not need to know which fields exist.

**Where applied.**
- [src/form/task/localization_patterns_widget.cpp](src/form/task/localization_patterns_widget.cpp) — `kMatchGroupSpecs` for `mtc::MatchGroupConfig`.
- [src/matching/match_config_property_adapter.cpp](src/matching/match_config_property_adapter.cpp) — pattern-level specs.

### 2.2 The table must mirror the struct exactly — both ways

**Rule.** When a member is added to a `Config` struct, the matching
`PropSpec` entry is mandatory. A missing entry silently drops the field
from the UI.

**Why.** The browser has no compile-time check that all fields are covered.
Forgetting one is a UX bug that's invisible in code review.

**Where applied.** This rule was the explicit ask of the Task 2 fix —
six missing specs were added to `kMatchGroupSpecs`.

---

## 3. Property-Widget Ownership Discipline

### 3.1 An adapter only deletes properties it created

**Rule.** A property-page adapter (e.g. `MatchConfigPropertyAdapter`) tracks
the exact `QtProperty*` instances it added and, in `destroy()`, deletes
only those — never `m_mgr->clear()` and never any property the host added.

**Why.** Calling `QtAbstractPropertyManager::clear()` deletes **every**
property in the shared manager, including properties owned by other panes
that happen to share it. This was a latent dangling-pointer bug.

**Where applied.**
- [src/matching/match_config_property_adapter.cpp](src/matching/match_config_property_adapter.cpp) — `destroy()` does `qDeleteAll(m_props); delete m_grpCommon; delete m_grpType;` and nothing else.

### 3.2 Pages that own group containers tear them down explicitly

**Rule.** A page that creates group properties (`m_groupVariant`, child
`m_groupProps[]`, etc.) must `qDeleteAll(...)` the children AND `delete`
the group container in its own rebuild path. Manager `clear()` cannot be
relied on — see 3.1.

**Where applied.**
- [src/form/task/localization_patterns_widget.cpp](src/form/task/localization_patterns_widget.cpp) — `rebuildPropertyBrowser` owns `m_groupVariant` lifecycle.

### 3.3 Edits are snapshotted before commit, rolled back on rejection

**Rule.** `onPropertyValueChanged` takes a snapshot of the model **before**
applying the new value. If the manager rejects the update (returns
non-OK `ManagerResult`), the snapshot is restored and the property is
re-set to the snapshot value with a signal blocker, leaving the UI
visually consistent with the model.

**Why.** Property editors mutate user intent directly into the widget;
without rollback, a rejected change leaves the UI showing a value the
model never accepted.

**Where applied.**
- `localization_patterns_widget.cpp::onPropertyValueChanged`.
- `localization_calibration_widget.cpp::onPatternConfigModified`.

---

## 4. Manager / Model Change Notification

### 4.1 The manager emits change signals — listeners react

**Rule.** Mutation methods on a manager (e.g. `PatternGroupManager`) must
emit `groupChanged(...)` / `patternChanged(...)` on **every** successful
mutation. Widgets do not re-emit; they subscribe and refresh.

**Why.** Centralising emission in the manager means a new caller cannot
forget to notify. It also avoids double-emit when multiple widgets edit
the same model.

**Where applied.**
- [src/matching/pattern_group_manager.cpp](src/matching/pattern_group_manager.cpp) — every successful path emits.

### 4.2 Non-QObject children do not emit

**Rule.** A model class that is not a `QObject` (e.g. `MatchGroup` —
held as a value/shared_ptr container, not a Qt parent) cannot emit signals
and must not pretend to. Notification responsibility lives one level up
in the owning manager.

**Where applied.**
- [src/matching/match_group.cpp](src/matching/match_group.cpp) — removed dead `emit configChanged()` call.

---

## 5. JSON Serialization

### 5.1 Owners delegate to the manager — they don't re-serialize children

**Rule.** A task's `toJson` / `fromJson` writes its own fields plus a
single nested object produced by the manager. The manager is responsible
for the schema of its children. The task is **not** allowed to iterate
groups/patterns itself.

**Why.** Two places writing the same schema diverges fast. Centralising
keeps round-trips correct and lets the manager evolve without touching
every caller.

**Where applied.**
- [src/model/task_localization.cpp](src/model/task_localization.cpp) — `toJson` writes `"patternManager"` by calling `m_patternManager->toJson()`; `fromJson` calls the corresponding loader.

### 5.2 Schema is documented at the producing function

**Rule.** Each `toJson` writes its schema in a comment block immediately
above the function. The matching `fromJson` references the same comment.
Schema lives next to the code that emits it, not in external docs.

**Where applied.**
- `pattern_group_manager.cpp` — `{ "groups": [ {…group fields…, "patterns": […]} ] }`.

### 5.3 Image BLOBs use a stable, parseable key format

**Rule.** Binary attachments associated with model objects use keys of the
form `g{groupNumber}_p{patternNumber}` and are parsed back via
`QRegularExpression` — never by index or position.

**Why.** Reordering groups or patterns at save/load time must not
disconnect a pattern from its image. Stable identifier-based keys survive
reorder.

**Where applied.**
- `task_localization.cpp::getTaskImageMap` / `loadTaskImageMap`.

### 5.4 The factory restores via the model's own `fromJson`

**Rule.** A `TaskFactory::create*` function delegates to `task->fromJson(obj)`
**once** and trusts it. Factories do not selectively restore parts of the
model (e.g. config-only). That leaks knowledge of the schema into the
factory and silently drops new fields.

**Why.** This was a real bug: `createTaskLocalization` was calling
`config.fromJson(...)` directly and dropping device IDs and the entire
pattern library on load.

**Where applied.**
- [src/model/task_factory.cpp](src/model/task_factory.cpp).

---

## 6. UI Composition Conventions

> **Moved.** UI structure/composition rules now live in
> [ui_design_rules.md](ui_design_rules.md) (the single source of truth for
> UI/QSS). See §1–§2 there for: nav/layout primitives authored in `.ui` (not
> code), grouped QSS selectors, removing orphan QSS with the widget, and
> programmatic uncheck of exclusive button groups.

---

## 7. Signal / Slot Plumbing

### 7.1 Signal-to-signal connections require matching signatures

**Rule.** A `connect(src, &Src::sig, dst, &Dst::sig)` direct signal
forward is only legal when the signatures match. Otherwise wire through
a lambda that synthesizes the missing arguments at emit time.

**Why.** Without the lambda the connection silently fails at runtime
(Qt logs a warning and drops it).

**Where applied.**
- `AddPatternWizard::requestCameraImage()` (no-arg) → `LocalizationPatternsWidget::requestCameraImage(QString)` — wired via lambda that reads `comboBox_camera->currentData().toString()`.

### 7.2 Combobox selections store IDs in `userData`, display labels in text

**Rule.** Device/camera/etc. comboboxes store the stable `QString` ID via
`QComboBox::addItem(label, id)` and are read back through `currentData()`.
Never parse the visible text.

**Why.** Localisation, label-change, and duplicate-name cases all break
text-parsing approaches. `userData` is stable.

**Where applied.**
- `localization_patterns_widget.cpp::onTriggerCameraClicked`.

### 7.3 Disable controls instead of swallowing emits

**Rule.** When the action behind a control is meaningless (empty combo,
no selection), disable the trigger button. Don't let it fire and then
no-op.

**Where applied.**
- `rebuildCameraCombo` disables the trigger button when no cameras are assigned.

---

## 8. Model / Device Resolution

### 8.1 Tasks resolve their assigned devices through the project

**Rule.** `ITask` exposes `assignedDevicesOfType(DeviceType)` which walks
`m_assignedDeviceIds`, calls `project()->deviceManager()->deviceById(id)`,
filters by `deviceType()`, and returns the typed list. Widgets never go
direct to the device manager.

**Why.** The task is the authority on which devices it owns. Widgets that
bypass it can show devices the task hasn't been assigned, breaking
encapsulation and multi-task isolation.

**Where applied.**
- [src/model/itask.cpp](src/model/itask.cpp) — helper.
- `localization_patterns_widget.cpp::rebuildCameraCombo` — uses helper.

### 8.2 Rebuilds are triggered by `devicesChanged` from the task

**Rule.** Widgets that depend on the task's device assignment subscribe to
`task->devicesChanged()` and rebuild their dependent UI from scratch. They
do not poll, and they do not assume the assignment is stable across the
widget's lifetime.

**Where applied.**
- `localization_patterns_widget.cpp` — `rebuildCameraCombo` is the
  registered slot.

---

## 9. Canvas / Coordinate Systems

### 9.1 Canvas state lives in image-pixel coordinates

**Rule.** All persistent shape state (crop rect, pick point, jaw box,
rotation) is stored in image-pixel coordinates. The view transform
(`m_zoom` + `m_offset`) maps to widget coordinates only at paint time and
in input handlers.

**Why.** Image-pixel state survives zoom and pan; widget-pixel state does
not. It also matches how downstream consumers (matchers, persistence,
spin-box widgets) expect coordinates.

**Where applied.**
- [src/form/pattern/pattern_canvas.cpp](src/form/pattern/pattern_canvas.cpp).

### 9.2 Signals carry image-pixel state, documented

**Rule.** Every canvas signal documents in its declaration that arguments
are image-pixel values. Receivers (e.g. spin boxes in the wizard) bind
directly without conversion.

**Where applied.**
- `pattern_canvas.h` — signal comments call out "image-pixel coords".

### 9.3 Pixmap rendering uses nearest-neighbour at zoom

**Rule.** `QPainter::SmoothPixmapTransform` is **off** for image canvases.
Vision UIs need pixel-exact feedback (which pixel is under the cursor);
smoothing hides per-pixel structure.

**Where applied.**
- `pattern_canvas.cpp::paintEvent` — `setRenderHint(QPainter::SmoothPixmapTransform, false)`.

### 9.4 Pick step shows the crop rect read-only

**Rule.** Steps downstream of "crop" show the crop rectangle as a read-only
dashed overlay. The user is informed of the crop, cannot edit it from the
later step, and pick coordinates are reported relative to the crop frame
(not the original image) when crop is active.

**Why.** Pick semantics are "where on the cropped pattern", not "where in
the source frame". Making this implicit produces sub-pixel offset bugs
between train and run.

**Where applied.**
- `pattern_canvas.cpp` — pick-mode draw + clamp.
- `add_pattern_wizard.cpp::pickX()` / `pickY()` — return crop-relative
  when `keepOriginal == false`.

### 9.5 Box (jaws) handles may extend outside the frame

**Rule.** Resize and rotate handles for the jaws box must remain
interactable even when the box extends past the image bounds. The
underlying state (`m_boxCx`, `m_boxCy`, `m_boxW`, `m_boxH`,
`m_boxAngle`) is unclamped.

**Why.** A jaws box is a robot-frame concept, not an image-frame concept;
it is legitimately allowed to extend off the visible image.

**Where applied.**
- `pattern_canvas.cpp` — box-mode handle logic.

### 9.6 Spin-box ↔ canvas sync is bidirectional and bounded

**Rule.** Widget spin boxes that mirror canvas state are updated through a
signal blocker on both sides. Their ranges are widened to the image size
on `imageSizeChanged`. Default values are clamped to the new range.

**Where applied.**
- `add_pattern_wizard.cpp::onImageSizeChanged`.

---

## 10. Persistence / Repository

### 10.1 Image BLOBs go through the repository, not raw SQL in models

**Rule.** Models expose `getTaskImageMap()` / `loadTaskImageMap()`; the
repository is responsible for actually writing/reading BLOBs in
`project_images`. Models never `QSqlQuery`.

**Why.** Keeps model classes pure and testable, and the repository is the
single place that knows the schema.

**Where applied.**
- `task_localization.cpp` exposes the two map methods.
- `ProjectRepository` performs the I/O.

---

## 11. Process / Workflow Rules

### 11.1 Confirm requirements before implementing non-trivial UI changes

**Rule.** When the user asks for a feature with more than one defensible
interpretation (e.g. "where do cameras come from for the combobox"),
respond with `AskUserQuestion` enumerating the options and recommended
defaults **before** writing code.

**Why.** Explicit user feedback during Task 12 — *"Hãy xác nhận yêu cầu
trước khi implement"*. Saves a rewrite.

### 11.2 Risky cleanups flagged, not silently applied

**Rule.** When a sanity check reveals an adjacent issue (e.g.
`showDeviceConfigPage` not unchecking `btn_nav_patterns`), the fix is
described in the response and applied only on the user's say-so.

**Why.** "Approval for X" does not extend to Y, even when Y is obviously
related. Match action scope to what was actually requested.

### 11.3 No emoji in code; markdown links for file refs

**Rule.** Code and comments contain no emoji. User-facing text in this
project uses markdown link syntax `[file.cpp:42](src/file.cpp#L42)` for
file references — never backticks or HTML.

### 11.4 Build outputs stay under `build/`, never at repository root

**Rule.** Any local build, rebuild, test build, or verification build must be
performed inside a dedicated subfolder under the repository's `build/`
directory, following the pattern `./build/<build-folder>/`. Do not generate
build artifacts directly in the repository root.

**Why.** Root-level build output pollutes the working tree, makes navigation
harder, and increases the chance of accidentally mixing generated files with
source-controlled files. Keeping every build in `build/...` makes cleanup,
comparison, and parallel debug/release or test/app builds predictable.

**Where applied.**
- Session rule added on 2026-05-31 for all future Codex build/test runs in this
  project.

---

## 12. Device Family Modules

### 12.1 Each device family lives in its own subfolder under `src/device/`

**Rule.** Each device family (camera, PLC/MC, vision output, robot, …)
lives in its own subfolder under `src/device/`. Header, cpp, config,
request, and every artifact of the family are colocated in that folder.

**Why.** Splitting the lifecycle by family means adding or modifying one
device does not touch another family's code, and any review of a single
device only needs to read one folder.

**Where applied.**
- [src/device/camera/](src/device/camera/), [src/device/plc/](src/device/plc/),
  [src/device/output_device/](src/device/output_device/) — each folder is
  one family.

### 12.2 Adding a new device type requires four synchronized touchpoints

**Rule.** When adding a new device type (or subtype), the following four
touchpoints must be updated in the same commit:

1. A new value in `enum DeviceType` at
   [idevice_config.h](src/device/idevice_config.h).
2. The display name (`DEVICE_TYPE_NAME_*`) plus the matching `case` arms
   in `DeviceTypeToString` / `DeviceTypeFromString`.
3. A `DeviceFactory::create<Name>(obj, parent)` method in
   [device_factory.cpp](src/device/device_factory.cpp).
4. A dispatch case in `DeviceFactory::create()` that routes from the
   enum value to the new factory method.

**Why.** Missing one touchpoint means JSON load/save "runs without
error" but the device silently becomes nullptr. That is a tracking
nightmare. Requiring all four places makes the omission visible in PR
review.

**Where applied.**
- `VisionOutputDevice` was added with all four touchpoints in
  [idevice_config.h](src/device/idevice_config.h),
  [device_factory.h](src/device/device_factory.h),
  [device_factory.cpp](src/device/device_factory.cpp).

### 12.3 Device configs are `Q_GADGET` with property macros

**Rule.** Every concrete `IDeviceCfg` must declare `Q_GADGET` and expose
its fields through the macros in
[qgadget_marco.h](src/qgadget_marco.h)
(`G_PROPERTY_STRING_READWRITE`, `G_PROPERTY_NUMBER_READWRITE`, etc.).
The family's JSON keys are centralised in
[idevice_config.h](src/device/idevice_config.h) with the prefix
`DEVICE_JSK_<FAMILY>_*`.

**Why.** The macros generate getter/setter plus meta-info so the
property browser can introspect the config. Centralising the keys
avoids collisions and keeps them searchable.

**Where applied.**
- [vision_output_config.h](src/device/output_device/vision_output_config.h) —
  `Q_GADGET` plus five property macros.
- `DEVICE_JSK_VOUT_*` lives in `idevice_config.h`.

### 12.4 Requests share a single `RequestType` enum

**Rule.** Each device family has one entry in `enum RequestType` at
[irequest.h](src/device/irequest.h) (e.g. `Request_VisionOutput`). The
request class overrides `type()` and `clone()` (which returns
`std::shared_ptr<IRequest>`).

**Why.** `IDevice::pushRequest(IRequest*)` dispatches on `type()`.
Without a dedicated enum value the device cannot tell its own family's
request apart from a foreign one.

**Where applied.**
- [vision_output_request.h](src/device/output_device/vision_output_request.h).

### 12.5 Promote to sub-type only when a second implementation appears

**Rule.** When a new device family has only one implementation, expose
it as a top-level value in `DeviceType` directly. Once a second
implementation appears that shares the same protocol family (e.g. two
camera vendors, two TCP output vendors), refactor into a sub-type
pattern à la `CameraType` (a sub-dispatcher `createCamera` selects
`BaslerGigE` / `Realsense` / …) instead of adding a new top-level enum
value for each variant.

**Why.** A top-level value per variant balloons `DeviceType` and forces
callers to distinguish at the wrong level (protocol vs. vendor). The
sub-type pattern keeps `DeviceType` at the "protocol family"
granularity and lets a sub-dispatcher handle vendors.

**Where applied.**
- `VisionOutputDevice` currently sits top-level
  (`DeviceType::VisionOutput`). When a second TCP/IP output device
  appears it will be refactored the same way Camera was, see
  [camera_device.h](src/device/camera/camera_device.h).

---

## 13. TCP / Network Patterns

### 13.1 Server-mode devices bind from config, one `QTcpServer` per logical port

**Rule.** A device acting as a TCP server pulls its bind address from
config (`QHostAddress(cfg.listenAddress)`, falling back to
`QHostAddress::Any` when null). Each "logical port" (e.g. main +
heartbeat) is its own `QTcpServer` with dedicated slots.

**Why.** Splitting servers by port concept lets each path own its logic
(timeout, parser, reject policy) without multiplexing on a single
socket. Reading the bind address from config makes multi-NIC deployment
a config-only change.

**Where applied.**
- [vision_output_device.cpp](src/device/output_device/vision_output_device.cpp) —
  `m_mainServer` + `m_hbServer`, each with its own
  `onMainNewConnection` / `onHeartbeatNewConnection` slot.

### 13.2 Single active client per port; reject pending duplicates

**Rule.** Accept at most one client per port at a time. If a pending
connection arrives while a client is already attached: log,
`disconnectFromHost()`, then `deleteLater()` the newcomer. Never
replace the in-flight client.

**Why.** Two clients on the same port means a reply can route to the
wrong one and cause state cross-talk. Rejecting early is far easier to
debug than a silent override.

**Where applied.**
- `onMainNewConnection` and `onHeartbeatNewConnection` in
  [vision_output_device.cpp](src/device/output_device/vision_output_device.cpp).

### 13.3 Heartbeat timeout measures time since last *valid reply*, not last *probe*

**Rule.** For heartbeat implementations, the `QElapsedTimer` that
measures timeout must only be `restart()`-ed inside the slot that
receives a **valid reply**. Never restart it inside the probe sender —
restarting on send keeps `elapsed()` strictly < interval < timeout, so
timeout never fires.

Initialisation: the timer is only `start()`-ed the first time (in the
sender or on connect) when it is not already valid; later sends leave
it alone.

**Why.** Lesson learned: the first version of `sendHeartbeatProbe()`
called `m_hbLastReplyTimer.restart()` every tick and the timeout test
failed forever. The correct semantics are "how long has the client been
silent" — only a valid reply proves the client is still alive.

**Where applied.**
- `VisionOutputDevice::sendHeartbeatProbe()` —
  [vision_output_device.cpp](src/device/output_device/vision_output_device.cpp)
  (only `start()` when not yet valid, never `restart()`).
- `handleHeartbeatPayload` `restart()`s the timer when it successfully
  parses `ack,...`.

### 13.4 Stream payloads are framed by a delimiter; parse with buffer + scan

**Rule.** Stream-based protocols (TCP) **always** use a delimiter for
framing (e.g. `;` for result `"N,x,y,z,r,...;"`, `.` for heartbeat
`"ack,N."`). The receiver maintains a `QByteArray` buffer, appends
every `readAll()` chunk, scans for the delimiter, and cuts out one
message at a time. Never assume one `readyRead` equals one message;
never rely on `bytesAvailable` unless the protocol is fixed-length.

**Why.** TCP can split, coalesce, and stretch segments freely. One
logical packet may arrive over three `readyRead`s; three logical
packets may land in one `readyRead`. Buffer + delimiter is the only
correct framing for variable-length protocols.

**Where applied.**
- `onMainReadyRead` (`;` delimiter) and `onHeartbeatReadyRead`
  (`.` delimiter) in
  [vision_output_device.cpp](src/device/output_device/vision_output_device.cpp).

---

## 14. Qt Test Subprojects

### 14.1 Tests live in their own `.pro` subprojects under `tests/`

**Rule.** Mỗi test target ở `tests/<name>/<name>.pro`, pull source từ
`$$PWD/../../src/...` và compile độc lập với app chính. Không thêm
source test vào `ncr_picking.pro`.

**Why.** Giữ `.pro` chính nhỏ; user chỉ build test khi cần; tránh phụ
thuộc `QtTest` lan ra binary production.

**Where applied.**
- [tests/vision_output_device_test/vision_output_device_test.pro](tests/vision_output_device_test/vision_output_device_test.pro) —
  pull source qua `ROOT_DIR = $$PWD/../..`.

### 14.2 `QTcpSocket::waitForReadyRead` is not enough when peer slot lives in same thread

**Rule.** Khi test client và server cùng main thread (kiểu unit test
QtTest), `QTcpSocket::waitForReadyRead(ms)` chỉ pump cho socket của
chính nó — slot `newConnection` / `readyRead` của server **không**
chạy. Viết helper xen `QCoreApplication::processEvents(AllEvents, ms)`
giữa các vòng chờ, hoặc dùng `QTest::qWait()` trước khi check state.

**Why.** Lesson learned: heartbeat tests fail "`waitReadable` returned
FALSE" vì server slot không có dịp xử lý `newConnection` để gửi probe.
Helper `waitReadable` phải pump event loop, không chỉ chờ socket.

**Where applied.**
- `waitReadable()` trong
  [tests/.../main.cpp](tests/vision_output_device_test/main.cpp) — vòng
  lặp xen `processEvents(20ms)` + `waitForReadyRead(20ms)` cho tới khi
  có byte hoặc hết timeout tổng.

### 14.3 Pump events after writing before asserting on peer state

**Rule.** Sau khi test client `write()` xuống server (hoặc ngược lại),
**bắt buộc** pump event loop trước khi `QCOMPARE` state của peer.
Pattern: vòng `processEvents` nhẹ chờ condition true hoặc timeout
ngắn — không assert tức thời.

**Why.** `write()` chỉ kick TCP send buffer; slot `readyRead` của peer
cần event loop để chạy. Assert ngay sau write → race condition, test
flaky.

**Where applied.**
- `test_heartbeat_happy_path` — vòng chờ `expectedAckCount() == 2`
  với `processEvents`.

---

## 15. Themed Form Stylesheets

> **Moved.** Theming and stylesheet rules now live in
> [ui_design_rules.md](ui_design_rules.md) (the single source of truth for
> UI/QSS). See there for: per-form `<form>_<theme>.qss` pairs and the global
> vs per-form layers (§3), the `reloadStyleSheet()` + `themeChanged` contract
> and repolish-after-`setProperty` (§4), the design-token palette (§5), the
> empty-`styleSheet`-property rule (§2.2), and dynamic-property variant styling
> (§3.4).
>
> One UI behaviour kept here because it is layout logic, not styling:
>
> ### 15.4 Hide the stack rather than show an empty page
>
> **Rule.** When a `QStackedWidget` drives type-conditional config and one type
> has no options, mark that type with `stackPage = -1` and hide the stack widget
> entirely. Do not add an empty page padded with a "no options" label.
>
> **Why.** Empty pages introduce phantom vertical space (the parent layout
> reserves the page height). Hiding the stack lets the surrounding layout
> collapse naturally.
>
> **Where applied.**
> - [src/form/add_device_wizard.cpp](src/form/add_device_wizard.cpp) — the
>   VisionOutput card has `stackPage = -1`; `selectCard()` calls
>   `ui->adwConfigStack->hide()`.

---

## 16. Documentation Language

### 16.1 All comments, markdown, and committed documentation are in English

**Rule.** Source-code comments, `.md` documents, header banners inside
`.qss` files, commit messages, and any text shipped in the repository
are written in English. Vietnamese (or any other language) only
appears in transient artefacts: chat messages, scratch notes,
ephemeral memory, in-PR conversation.

**Why.** A single working language for committed text keeps the
project searchable, AI-tooling-friendly, and accessible to
contributors who do not read Vietnamese. Mixed-language comments are
also a frequent source of mojibake on Windows when encoding metadata
is missing.

**Where applied.** Going forward, every new rule entry in this
document and every new source comment. Sections 12 and 13 were
translated retroactively when this rule was introduced; section 14
remains in Vietnamese pending a follow-up pass (flagged, not
silently rewritten — see Rule 11.2).

---

## 17. UML Documentation Maintenance

### 17.1 Structural code changes must update `uml/`

**Rule.** Any change that modifies the code organization or architecture must
update the corresponding PlantUML source under [uml/](uml/) in the same task.
This includes adding, removing, renaming, or moving modules, device families,
device sub-types, task types, runtime runners, persistence ownership, major UI
containers, or shared model relationships. If the change intentionally does not
affect the diagrams, say so in the response.

**Why.** The `uml/` folder is the current architecture reference and replaces
the historical `uml_diagram/` folder. Letting diagrams drift makes onboarding
and design review unreliable, especially in this project where runtime
threading, device dispatch, and industrial I/O ownership are safety-critical
design surfaces.

**Where applied.**
- [uml/README.md](../uml/README.md) plus the PlantUML files in [uml/](../uml/).

---

## Cross-Reference Map

| Rule | Primary file |
|---|---|
| 1.x  Property managers | [custom_property_managers.h](src/widgets/property_browser/custom_property_managers.h) |
| 2.x  PropSpec tables | [localization_patterns_widget.cpp](src/form/task/localization_patterns_widget.cpp) |
| 3.x  Adapter ownership | [match_config_property_adapter.cpp](src/matching/match_config_property_adapter.cpp) |
| 4.x  Manager signals | [pattern_group_manager.cpp](src/matching/pattern_group_manager.cpp) |
| 5.x  JSON delegation | [task_localization.cpp](src/model/task_localization.cpp), [task_factory.cpp](src/model/task_factory.cpp) |
| 6.x  UI composition | → [ui_design_rules.md](ui_design_rules.md) §1–§2 |
| 7.x  Signal plumbing | [localization_patterns_widget.cpp](src/form/task/localization_patterns_widget.cpp) |
| 8.x  Device resolution | [itask.cpp](src/model/itask.cpp) |
| 9.x  Canvas | [pattern_canvas.cpp](src/form/pattern/pattern_canvas.cpp), [add_pattern_wizard.cpp](src/form/pattern/add_pattern_wizard.cpp) |
| 10.x  Persistence | [task_localization.cpp](src/model/task_localization.cpp) |
| 11.x  Workflow | (this document) |
| 12.x  Device family modules | [idevice_config.h](src/device/idevice_config.h), [device_factory.cpp](src/device/device_factory.cpp), [vision_output_config.h](src/device/output_device/vision_output_config.h) |
| 13.x  TCP / network patterns | [vision_output_device.cpp](src/device/output_device/vision_output_device.cpp) |
| 14.x  Qt Test subprojects | [tests/vision_output_device_test/](tests/vision_output_device_test/) |
| 15.x  Themed form stylesheets | → [ui_design_rules.md](ui_design_rules.md) §3–§6 |
| 16.x  Documentation language | (this document) |
| 17.x  UML maintenance | [uml/README.md](../uml/README.md), [uml/](../uml/) |
