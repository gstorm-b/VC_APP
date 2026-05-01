# NCR Vision — UI Design Handoff (Qt6 / C++)

> **Target stack:** Qt 6, C++, Qt Widgets + QSS (following existing ncr_picking codebase)  
> **Fidelity:** High-fidelity — recreate pixel-accurately in Qt Widgets  
> **Design files:** HTML/JSX prototypes — open in browser as visual reference. Do NOT ship HTML.

---

## 1. Design Tokens

### Colors
```cpp
// QSS / QColor values
Background:       #0b0f17   QColor(11,15,23)
Surface:          #0e1520   QColor(14,21,32)
Surface deep:     #060d18   QColor(6,13,24)
Border:           #1a2540   QColor(26,37,64)
Border active:    #243044   QColor(36,48,68)
Text:             #dce8f5   QColor(220,232,245)
Text secondary:   #6b7ea0   QColor(107,126,160)
Text muted:       #3a4f6a   QColor(58,79,106)
Accent blue:      #2b8ce8   QColor(43,140,232)
OK green:         #22d17a   QColor(34,209,122)
Warning amber:    #f5a623   QColor(245,166,35)
Error red:        #e84040   QColor(232,64,64)
Status bar bg:    #2b8ce8   (blue strip)
```

### Task type colors
```cpp
LocalizationTask: #2b8ce8   chip text: "LOC"
PickPlaceTask:    #22d17a   chip text: "PICK"
InspectTask:      #f5a623   chip text: "INSP"
```

### Device type colors
```cpp
Camera:  #2b8ce8   chip: "CAM"
PLC:     #22d17a   chip: "PLC"
Robot:   #f5a623   chip: "BOT"
```

### Typography
```
UI:   "Segoe UI" (Windows) — sizes 9/10/11/12/13/14pt
Mono: "Consolas" (Windows) — for addresses, values, device names, register data
Section labels: 9pt, weight 700, UPPERCASE, letter-spacing 1px
Body: 10–11pt, weight 400–600
KPI values: 14–18pt, weight 800, mono
```

### Geometry
```
Top bar height:         44px
Task tab bar height:    36px
Sidebar width:          248px
Device nav width:       188px
Property panel width:   268px  (collapsible to 28px)
Bottom dock default:    180px  (min 120px, max 400px)
Status bar height:      24px
Border radius (inputs): 4px
Border radius (cards):  6–8px
Border radius (modals): 8–10px
```

---

## 2. Application 1 — Config App

### Main Window (QMainWindow)

```
QMainWindow
├── Custom top bar widget (44px, background #060d18)
├── centralWidget → QWidget
│   └── QHBoxLayout (margin 0, spacing 0)
│       ├── Sidebar widget (248px fixed width)
│       └── DockWorkspace widget (flex, QSplitter or QTabWidget-based)
└── Custom status bar (24px, background #2b8ce8, white text)
```

Menus: use `QMenuBar` hidden behind the custom top bar — render menu items as `QToolButton`s in the custom bar, popping up `QMenu` on click.

---

### Sidebar (248px)

Use `QTreeWidget` or custom `QAbstractItemView` with three-level hierarchy:

**Level 1 — Project** (background `#0e1520`)
- Folder icon (16px, accent blue)
- Project name (12pt bold, `#dce8f5`)
- Collapse/expand chevron
- [+] button (add task) — visible on hover

**Level 2 — Task** (indent 8px)
- Collapse chevron
- Number badge `[N]` (18×18px rounded rect, amber `#f5a623` border/text)
- Task icon 13px
- Task name (12pt JetBrains Mono / Consolas)
- Type chip: colored mini-badge (LOC/PICK/INSP)
- Actions on hover: ⚡ Set active, [+] Add device, 🗑 Delete

**Level 3 — Device** (indent 20px from task level)
- Device icon 13px (colored per type)
- Connection dot (6px circle): green=connected, red=disconnected
- Device name (11pt Consolas)
- Type chip (8pt, colored)
- Actions on hover: → Move, × Remove

**Active node:** `#1a2a45` background, `2px solid #2b8ce8` left border  
**Hover:** `#131c2e` background

**Double-click device** → emit signal to navigate workspace to that device's panel.

---

### Task Tab Bar

Implement as custom widget over `QTabBar` (document mode) or full custom paint:

- Tab size: min 140px, max 220px
- Each tab contains: type chip + task name + device count label + × button
- Active: background `#0b1524`, top border `2px solid {task-color}`
- Inactive: transparent, hover `#090f1c`
- [+] button opens `QListWidget` popup listing available tasks

---

### Device Navigation Panel (188px, QWidget left of content)

Sections (QVBoxLayout, spacing 0):

1. **Task header** (background `#060d18`, padding 10px 12px)
   - "ACTIVE TASK" label (9pt, `#1f2d42`)
   - Task name (11pt Consolas bold, `#dce8f5`)
   - Task type chip

2. **Status lamps row** (padding 8px, spaced evenly)
   - 4 lamps: READY / CAM / PLC / BOT
   - Each: 28–32px circle QLabel with custom paintEvent
   - OK: radial gradient green `#22d17a` + box-shadow effect (use QGraphicsDropShadowEffect)
   - Error: red `#e84040`
   - Label: 8pt uppercase below circle

3. **[Dashboard] button** (fixed, always first)
   - Active: `#152035` bg, `2px solid #2b8ce8` left border
   - Icon 14px + "Dashboard" 11pt

4. **Devices section header** (9pt label + [+] button)
5. **Device list** (QListWidget, custom delegate)
   - Each item: device icon + name + type sublabel + connection dot
   - Active: `{color}15` bg, `1px solid {color}44` border
   - Hover shows → (move) and × (delete) buttons

6. **[Settings] button** (fixed, always last, border-top)

---

### Content Area (flex, QStackedWidget)

Breadcrumb bar (36px, background `#0a1118`):
- Format: `TaskName ▸ DeviceName ● CONNECTED`
- Right: save + refresh QToolButtons

Content switches on device nav selection:
- **null** → Dashboard panel
- **camera device** → Camera panel
- **plc device** → PLC panel
- **robot device** → Robot panel
- **`__settings`** → Settings panel

---

### Property Inspector Panel (268px, right of content)

```
QWidget (vertical)
├── Header row: "Properties" label + [changed count badge] + [collapse ×]
├── Context row: colored dot + device/task name + type label
├── Search input (QLineEdit, 11pt, icon prefix)
├── Scrollable property tree
│   └── PropGroup (collapsible, QTreeWidget rows or custom)
│       └── PropRow: label | value input
└── Description footer (2–3 lines, 10pt, `#3a5070`)
└── [Apply] [Revert] buttons (only when unsaved changes)
```

PropRow grid: `gridTemplateColumns: 1fr 1fr` → Qt: `QFormLayout` or `QGridLayout`

**Input types:**
- `string` → `QLineEdit`, background `#090e18`, border `#1a2540`
- `number` → `QDoubleSpinBox` / `QSpinBox`, same styling, unit label after
- `bool` → Custom toggle widget (30px wide pill) or `QCheckBox` restyled
- `select` → `QComboBox`
- `readonly` → `QLabel`, color `#2a3a52`

**Changed state:** amber left border `2px solid #f5a62355`, input text `#f5a623`

**Schema groups per device type:**

*Camera:* Connection | Acquisition (Exposure, Gain, Auto, Trigger, Pixel Format, FPS) | ROI | Advanced  
*PLC:* Connection | Input Signal Map (Trigger/Reset/Abort bit addresses) | Output Signal Map (Ready/Complete/OK/NG bits) | Data Output Registers (D100–D105)  
*Robot:* Connection | Coordinate Transform (scale X/Y, offsets X/Y/Z, rotation) | Operation  
*Task (no device):* Matching (threshold/maxResults/angleRange/scaleTol/overlap/minContrast) | Execution (retry, logLevel, saveNG, autoStart) | Calibration  

---

### Bottom Dock (QDockWidget, BottomDockWidgetArea)

Resizable (drag handle = top border, cursor `SizeVerCursor`):

**Tab bar** (28px, background `#060d18`):
- "System Log" tab + "PLC Monitor" tab
- Active: `#0b1524` bg, top border `2px solid #2b8ce8`
- [×] close button right side

**System Log content:**
- Monospaced table: time(56px) | level(34px, colored) | message
- Filter buttons: ALL / OK / WARN / ERROR
- Level colors: OK=`#22d17a`, INFO=`#5ba3f0`, WARN=`#f5a623`, ERROR=`#e84040`
- Qt: `QPlainTextEdit` with custom QSyntaxHighlighter, or `QTableWidget` (faster for large logs)

**PLC Register Monitor:**
- Live-updating table: Address | Name | Value | Type | Unit
- Bit values shown as ON/OFF pills (green/dim)
- Word values as plain numbers
- LIVE/OFFLINE status indicator
- Qt: `QTableWidget` with timer-driven update, custom `QStyledItemDelegate` for ON/OFF cells

---

### Dialogs

**New Project** (`QDialog`, 480px):
- Project Name `QLineEdit`
- Description `QPlainTextEdit` (100px height)
- [Cancel] [Create Project] buttons

**New Task** (`QDialog`, 520px):
- Task Name `QLineEdit` (Consolas font)
- Task Type — radio cards (visual selection, 3 options)
- Save Path `QLineEdit` + [Browse] button

**Add Device** (`QDialog`, 540px):
- Device type picker — 3-column card grid (Camera / PLC / Robot)
- Device Name `QLineEdit`
- Info strip (warning icon + text)
- [Cancel] [Add Device] buttons (button color matches device type)

**Move Device** (`QDialog`, 440px):
- List of available destination tasks (selectable rows)
- [Cancel] [Move Device] buttons

---

### Floating Windows

Use `QWidget` with `Qt::Tool | Qt::FramelessWindowHint`, custom title bar:
- Title bar: 32px, solid colored bg (task type color)
- Drag via `mousePressEvent` / `mouseMoveEvent`
- [DOCK] button → re-dock to grid
- macOS-style close dot (red circle)

---

## 3. Application 2 — Runtime App

### Main Window

```
QMainWindow (or QWidget fullscreen)
├── Top bar (48px, background #040a12, bottom border 2px accent blue)
├── Alarm bar (conditional, 36px, red/amber bg)
├── Task grid (flex, QGridLayout 2×2 default)
├── Event log strip (resizable, bottom)
└── Floating task windows (QWidget, Qt::Tool)
```

### Top Bar (48px)

- Logo + "NCR**vision** RUNTIME" (two-line: main name + "RUNTIME" subtext 9pt)
- System status pill: "SYSTEM OK" (green) or "WARNING" (amber)
- Device count label
- Live clock (`QTimer` 1s, 18pt Consolas bold, white)
- [Light/Dark] theme toggle button
- [Config →] link button

### Alarm Bar

Show only when alarms exist:
- Background: red or amber (depending on severity)
- Each alarm chip: time + task name + message + × dismiss
- [ACK ALL] button

### Task Monitor Card (4 per grid, 2×2)

Each card is `QGroupBox` or custom `QFrame`:

**Header (background `#060d18`):**
- Type chip + task name (Consolas 14pt bold) + line label
- Status badge: RUNNING (green) / STOPPED (dim) / ERROR (red pulse)
- Float window button + Fullscreen button

**Device pills row:**
- CAM / PLC / BOT pills with connection dots

**Stats row (4 cells, borderRight dividers):**
- Total Runs | OK Rate | Cycle (ms) | Last Score
- Colors reactive: OK Rate green≥95%, amber≥90%, red<90%

**Camera feed area (flex height):**
- Dark bg `#030810`, grid lines, corner brackets, center reticle SVG
- Bottom overlay: match result (OK/NG, score, X, Y, angle)
- Top-right: ● LIVE / ○ PAUSED indicator
- Qt: `QLabel` with `QImage` paintEvent, overlay via `QPainter`

**Sparkline (28px height):**
- SVG polyline → Qt: custom `QWidget::paintEvent` using `QPainterPath`

**Action bar:**
- [STOP] red | [START] green | [RESET] amber | [TRIGGER] blue | [ACK] red (on error)
- Button states: disabled when task not running, color-coded

### Event Log Strip (resizable, bottom)

- Drag handle top edge
- Collapse/expand toggle
- Mono table: [level] + message
- Level coloring same as config app

### Floating Task Card

- `QWidget`, `Qt::Tool`
- Colored title bar (task type color), drag by title bar
- [DOCK] button to return to grid
- Full `TaskMonitorCard` inside

### Theme System

Use CSS variables pattern via QSS:
- Store theme in `QSettings` or `AppSettings`
- Apply via `qApp->setStyleSheet(loadQSS(themeName))`
- Dark: existing dark.qss extended
- Light: new light.qss (white/light gray surfaces, dark text)
- Expose `ThemeManager::setTheme(name)` signal → all widgets re-polish

---

## 4. Pattern Manager (Localization Task)

This panel replaces the `LocalizationPatternsWidget` in the existing codebase.

### Data Model (already implemented in C++)
```
PatternGroupManager
  └── QList<MatchGroup>
        ├── groupName (wstring, unique in manager)
        ├── groupNumber (int, unique in manager) ← runtime selector
        └── QList<MatchPattern>
              ├── patternName (wstring, unique in group)
              ├── patternNumber (int, unique in group) ← output info
              └── MatchPatternConfig
                    ├── minScore, angle, toleranceAngle, maxOverlap
                    ├── pickPosition (Point2f)
                    └── EdgeMatchConfig
                          ├── threshLower, threshUpper, kernelSize
                          ├── blurWidth, blurHeight, greediness
                          ├── minReduceLength, tSamples
                          └── invertBinaryThreshold, subPixelEstimation, stopAtLayer1
```

### Layout (3-column horizontal splitter)

```
[Monitor pane (flex)] | [Pattern Library (260px)] | [Inspector (240px)]
```

**Monitor pane:**
- Toolbar (44px): [▶ Trigger Camera] [📂 Open Image] [⊞ Set Workspace] | Active Group dropdown | [Show ROI] [Use ROI] | status pill | [▶ Run Matching]
- Image viewer (QGraphicsView, ImageViewOnly widget — already exists)
- Raw / Result tab toggle
- KPI strip (78px): POSSIBLE OBJECTS | EXECUTION TIME | BELOW THRESHOLD | PICKING POSITION (X/Y/Z/R)

**Pattern Library tree (260px, PatternTreeWidget):**
```
▼ [1] Part_A  (group number badge, amber)           [+pat][🗑]
    [1] Front_face  ● 0.90  120×96                  [🗑]
    [2] Side_face   ● 0.88  88×110                  [🗑]
    [3] Top_view    ○ 0.90  —                       [🗑]
▶ [2] Part_B                                        [+pat][🗑]
▶ [3] Part_C (empty)                                [+pat][🗑]
[+ New Group]
```

- Group number badge: amber `#f5a623`, tooltip "Runtime group selector"
- Pattern number badge: blue `#5ba3f0`, tooltip "Output pattern ID"
- Connection dot: green=learned, red=not learned
- Double-click name → inline rename (`QLineEdit` delegate)
- Double-click number badge → renumber dialog
- Context menu: Rename / Renumber / Delete / Set as Active

**Inspector pane (240px, vertical splitter):**
- Top: Pattern thumbnail (QGraphicsView, 120px min height)
  - Shows pattern image with pick position dot overlay
  - "No pattern selected" placeholder
- Bottom: Match Configuration (`QtPropertyBrowser` — already in codebase)
  - Uses existing `MatchConfigPropertyAdapter`

### Add Group Dialog
- Group Name field (must be unique across manager)
- Group Number field (must be unique, integer ≥ 1)
- Helper text: "Used at runtime to select this group"
- Validation: red border + error message on duplicate

### Add Pattern Dialog
- Pattern Name field (unique within group)
- Pattern Number field (unique within group, integer ≥ 1)
- Helper text: "Reported as pattern ID in output data"
- Note: image capture/load happens after creation

### Active Group Indicator
- In tree: glowing blue dot on active group row
- In toolbar: dropdown shows active group, labeled with number
- Runtime: operator sends group number via PLC → manager activates matching group

---

## 5. Existing Components to Reuse

| Design element | Existing Qt widget | File |
|---|---|---|
| Image viewer (monitor) | `ImageViewOnly` / `ImageWidget` | `src/widgets/image_widget/` |
| Property browser | `QtPropertyBrowser` + adapter | `src/widgets/qtpropertybrowser/` |
| Pattern tree | `PatternTreeWidget` | (to be created, extend `QTreeWidget`) |
| Camera config | `BaslerCameraWidget` | `src/form/camera/` |
| PLC device | `McProtocolDeviceWidget` | `src/form/plc/` |
| System log | `SystemLogForm` | `src/system_log_form` |
| Theme (dark) | `dark.qss` | `resrc/styles/` |
| Icons | SVG resources | `resrc/icon/` |
| Docking system | `ads::CDockManager` | `3rdparty/advance_docking/` |

> **Key note:** The project already includes **Qt Advanced Docking System** (`advance_docking`). Use `ads::CDockManager` for all dock panels — task workspace, bottom log dock, property panel. This gives free floating, tabbed, and docked layouts.

---

## 6. Files in This Package

```
design_handoff/
├── README.md                    ← This file (full spec)
├── NCR Vision Config.html       ← Config app interactive prototype
├── NCR Vision Runtime.html      ← Runtime app interactive prototype
├── components/
│   ├── Icons.jsx                ← All SVG icon definitions
│   ├── Sidebar.jsx              ← Project explorer tree
│   ├── Dialogs.jsx              ← New project/task/device/move dialogs
│   ├── Dashboard.jsx            ← Dashboard panel (stats, charts, log)
│   ├── Panels.jsx               ← Vision, PLC, Camera, Robot, Settings panels
│   ├── PropertyPanel.jsx        ← Property inspector (schema, inputs, groups)
│   ├── TaskWorkspace.jsx        ← Task workspace (device nav + content)
│   ├── DockArea.jsx             ← Tab bar, bottom dock, floating windows
│   └── App.jsx                  ← Main app state + layout
└── runtime/
    └── RuntimeApp.jsx           ← Full runtime app implementation
```

---

## 7. Implementation Priority

1. **Design system** — QSS tokens (colors, fonts, spacing, border-radius) for both dark and light themes
2. **MainWindow shell** — top bar, sidebar, status bar
3. **Sidebar tree** — project/task/device nodes, selection, context menus
4. **Task workspace** — device nav panel + content stack + property panel (3-column splitter)
5. **Dashboard panel** — stats, device status rows, system log
6. **Device panels** — Camera config, PLC panel (register tables), Robot panel
7. **Dock system** — task tabs, bottom dock (log + PLC monitor) using `ads::CDockManager`
8. **Dialogs** — New project, new task, add device, move device
9. **Pattern Manager** — tree + inspector + monitor toolbar
10. **Runtime app** — task grid cards, alarm bar, event log, floating windows, theme toggle

---

*Open the HTML files in a browser (Chrome/Edge recommended) to explore all interactions before implementing.*
