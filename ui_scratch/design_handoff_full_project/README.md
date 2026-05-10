# Handoff: NCR Vision Config — Full Project

## Overview

**NCR Vision Config** is an industrial computer vision configuration platform for factory automation. It lets engineers configure, monitor, and operate computer vision tasks that guide robot bin-picking, pick-and-place, and surface inspection on assembly lines.

The UI follows a VS Code–style IDE shell: left sidebar project tree, central tabbed workspace, right property inspector, bottom log dock. Everything targets a dark 1920×1080 desktop display.

---

## About the Design Files

The files in this bundle are **high-fidelity HTML/JSX prototypes** — design references, not production code. The task is to **recreate these designs in the target codebase** using its established framework, state management, and component library. Lift exact design tokens (colors, spacing, typography) from this document; adapt all logic to the real data layer.

## Fidelity

**High-fidelity.** All colors, typography, spacing, component variants, hover/active/focus states, and interaction patterns are finalised. The developer should recreate the UI pixel-accurately.

---

## Application Shell

### Layout (top-level)

```
┌─────────────────────────────────────────────────────────┐
│  Top bar (44px)                                         │
├──────────┬──────────────────────────────────────────────┤
│          │  Tab bar (36px)                              │
│ Sidebar  ├──────────────────────────────────────────────┤
│ (248px)  │  Task Workspace (flex: 1)                    │
│          │  ├─ Device Nav (188px)                       │
│          │  └─ Content + Property Panel                 │
│          ├──────────────────────────────────────────────┤
│          │  Bottom Dock (resizable, default 180px)      │
├──────────┴──────────────────────────────────────────────┤
│  Status bar (24px)                                      │
└─────────────────────────────────────────────────────────┘
```

### Top Bar (44px)

- Background: `#1e1e1e`, bottom border: `1px solid #2d2d2d`
- Left: Logo mark (SVG) + wordmark `NCR` + `vision` (accent blue)
- Menu items: File, View, Access Level, Help — 12px, `#7a7a7a`, hover → `#cccccc`
- Right: CAM / PLC / ROBOT connection status pills + Admin role badge

**Connection pill** (per device type):
- Connected: bg `#22d17a0e`, border `#22d17a2a`, dot `#22d17a` glowing, text `#22d17a`
- Disconnected: bg `#e840400e`, border `#e840402a`, dot `#e84040`, text `#e84040`

**Admin badge**: bg `#2d2d2d`, border `#3c3c3c`, radius 5, `⬡ Admin` — icon `#f5a623`

### Status Bar (24px)

- Background: `#1563b5` (VS Code blue)
- Device count + connected count (10px, `rgba(255,255,255,0.9)`)
- Open task count
- Right: version string `NCR Vision Config v2.3.0` in `JetBrains Mono`

---

## Design Tokens

| Token | Value | Usage |
|-------|-------|-------|
| `bg` | `#1e1e1e` | Page background, panels |
| `surf` | `#252526` | Cards, headers |
| `surf2` | `#2d2d2d` | Elevated modals |
| `bd` | `#3c3c3c` | Default borders |
| `bd2` | `#454545` | Input borders, hover borders |
| `txt` | `#cccccc` | Primary text |
| `txt2` | `#9a9a9a` | Secondary text |
| `txt3` | `#7a7a7a` | Muted / hint text |
| `txt4` | `#5a5a5a` | Disabled text |
| `txt5` | `#454545` | Placeholder / label text |
| `acc` | `#2b8ce8` | Primary accent (blue) |
| `ok` | `#22d17a` | Success / connected green |
| `warn` | `#f5a623` | Warning orange |
| `err` | `#e84040` | Error red |
| `output` | `#9cdcfe` | PLC output ID (light blue) |
| Font sans | `Space Grotesk, sans-serif` | UI labels, buttons |
| Font mono | `JetBrains Mono, monospace` | Code, addresses, values |

---

## Component Inventory

### 1. Sidebar (`Sidebar.jsx`)

**Width**: 248px, min 200px, max 300px  
**Background**: `#252526`, right border `1px solid #3c3c3c`

**Header** (38px):
- Label `"EXPLORER"` — 10px, weight 700, `#7a7a7a`, uppercase, letterspacing 0.1em
- Action buttons: New Project, Open Project, Save (14px icons, `#9a9a9a`, hover → `#cccccc` + `#454545` bg)

**Hint bar**: `"Double-click device to open · Drag to move"` — 9px, `#5a5a5a`

**Project node**:
- Chevron (rotates on expand) + Folder icon (`#2b8ce8`) + project name (12px, weight 600, `#cccccc`)
- Hover: bg `#1a2235`
- `+` button to add task (appears on hover)

**Task node** (indent 8px from project):
- Chevron + Task icon + task name (12px, `JetBrains Mono`, weight 600 when active)
- Type chip: 3-letter code (LOC/PICK/INSP), 8px, weight 800, color-coded
- Active: bg `#1a2a45`, left border `2px solid #2b8ce8`
- `+` icon (add device) + trash icon (delete task) on hover

**Device node** (indent 18px from task):
- Device type icon + device name (11px, `JetBrains Mono`) + type badge (CAM/PLC/BOT)
- Active: bg `#1a2a45`, left border `2px solid {device color}`
- × delete button (transparent → `#e84040` on hover)

**Footer**: `v2.2.0 · NCR Vision` — 10px, `#7a7a7a`, `JetBrains Mono`

**Task type colors**:
| Type | Color |
|------|-------|
| LocalizationTask | `#2b8ce8` |
| PickPlaceTask | `#22d17a` |
| InspectTask | `#f5a623` |

---

### 2. Tab Bar (`DockArea.jsx` → `TaskTabBar`)

- Height 36px, bg `#1e1e1e`, bottom border `1px solid #2d2d2d`
- Each tab: type chip + task name (11px, `JetBrains Mono`) + device count + × close
- Active: bg `#252526`, top border `2px solid {task color}`, bottom border matches bg (selected look)
- Min width 140px, max 220px
- `+` button to open task picker dropdown

---

### 3. Task Workspace (`TaskWorkspace.jsx`)

**Left device nav** (188px, bg `#1e1e1e`, right border `1px solid #2a2d2e`):

- **Task header**: task name (11px mono bold) + type chip
- **Status lamps** (4 circles — READY, CAM, PLC, BOT):
  - Active: radial gradient green/red + glow shadow
  - Label: 8px, uppercase, `#5a5a5a`
- **Fixed nav tabs**: Dashboard, Patterns (LocalizationTask only), Devices section, Settings
  - Active: bg `#152035`, left border `2px solid #2b8ce8`, color `#2b8ce8`
  - Hover: bg `#2a2d2e`, color `#9a9a9a`
- **Device list items**: icon (with connected dot) + name + type label
  - Active: bg `{color}15`, border `1px solid {color}44`
  - Move arrow + delete × on hover
- **Settings tab** pinned at bottom, top border

**Breadcrumb bar** (36px, bg `#252526`):
- Task name → device name, color-coded
- Connected dot badge
- Save + Refresh icon buttons (right)

**Content area**: `flex: 1` — renders the panel for active selection

**Property panel** (268px, right, collapsible to 28px)

---

### 4. Dashboard (`Dashboard.jsx`)

Scrollable column layout, `padding: 20px 24px`.

**Stats row** (4-column grid):
- Cards: bg `#252526`, border `#3c3c3c`, radius 8, padding `16px 20px`
- Label: 10px uppercase `#9a9a9a`; Value: 28px weight 700, `JetBrains Mono`; Sub: 11px `#7a7a7a`

**Charts row** (2-column grid):
- SVG line chart with area gradient fill (`#2b8ce8` → transparent)
- `polyline` stroke `#2b8ce8`, strokeWidth 1.8

**Device status cards** (flex column):
- Per-device row: connection dot + name + detail + status badge + Connect/Disconnect button
- Border color matches connection state (green/red)

**System log** (flex: 1):
- Rows: time (`#7a7a7a`) + level (color-coded) + message (`#bfbfbf`), `JetBrains Mono` 11px
- Level colors: OK=`#22d17a`, INFO=`#9cdcfe`, WARN=`#f5a623`, ERROR=`#e84040`

---

### 5. Property Panel (`PropertyPanel.jsx`)

**Width**: 268px  
**Background**: `#1e1e1e`, left border `#3c3c3c`

Context-aware schema based on device type (camera/plc/robot) or task type (LocalizationTask/generic).

**Sections (collapsible)**:
- Section header: bg `#252526`, borders, chevron, label (10px, `#7a7a7a`, uppercase), prop count
- Hover: `#094771`

**Property row** (grid 1fr 1fr):
- Label: 11px, `#9a9a9a`, weight 500
- Input types: string, number (with unit), bool (toggle), select, readonly
- Focused: bg `#094771`, border `#2b8ce8`
- Changed: left border `2px solid #f5a62355`, value color `#f5a623`

**Search bar**: with loupe icon, clears with ×

**Footer**: shows focused prop key (`#2b8ce8`) + description (`#7a7a7a`)

**Apply / Revert** buttons appear when `changedCount > 0`

**Collapsed state**: 28px strip with hamburger icon; orange badge shows unsaved count

**Property schemas** (key groups per device type):

| Device | Groups |
|--------|--------|
| camera | Connection, Acquisition, ROI, Advanced |
| plc | Connection, Input Signal Map, Output Signal Map, Data Output Registers |
| robot | Connection, Coordinate Transform, Operation |
| LocalizationTask | Matching, Execution, Calibration |
| generic task | Execution |

---

### 6. Vision Panel (`Panels.jsx` → `VisionPanel`)

Left: camera feed (flex: 1) with corner brackets, center reticle, mode buttons (LIVE/CAPTURE/FREEZE), match result bar.

Right (280px): Pattern library grid (2-col `PatternCard` thumbnails) + Match Parameters form.

**PatternCard**:
- Grid bg + reticle SVG placeholder + name (11px mono) + size + score
- Selected: bg `#094771`, border `#2b8ce8`
- Disabled patterns: dark overlay with "DISABLED" label

---

### 7. PLC Panel (`Panels.jsx` → `PLCPanel`)

Connection config (3-col grid: IP, Port, Timeout) + Connect/Disconnect button with status dot.

Two register tables:
- **M Devices** (bit): ON/OFF chip badge + ON/OFF/TOGGLE action buttons
- **D Devices** (word): numeric value + editable input + Write button

Address column: `#9cdcfe` (output blue), 11px `JetBrains Mono`

---

### 8. Camera Panel (`Panels.jsx` → `CameraPanel`)

Left (260px): Camera list cards (model, IP, serial, resolution, fps) with connection dot + Scan Network button.

Right: Acquisition Settings form (Exposure, Gain, Trigger Mode, Pixel Format, Binning H/V) + ROI section (Offset X/Y, Width, Height).

---

### 9. Settings Panel (`Panels.jsx` → `SettingsPanel`)

Two cards: **Task Configuration** (name, type, comm device, output device) + **Execution Settings** (retry count, delay, log level, toggles for save-NG and auto-start).

---

### 10. Robot Panel (`Panels.jsx` → `RobotPanel`)

Connection config + **Coordinate Mapping** form (Scale X/Y, Offset X/Y/Z, Rotation).

---

### 11. Bottom Dock (`DockArea.jsx` → `BottomDock`)

- Resizable (drag handle, min 120px, max 400px)
- Two tabs: **System Log** + **PLC Monitor**
- System Log: time + level + message rows, Clear + filter buttons (ALL/OK/WARN/ERROR)
- PLC Monitor: live-updating register table (M bits as ON/OFF, D words as numbers), Connect/Stop

---

### 12. Floating Windows (`DockArea.jsx` → `FloatingWindow`)

Draggable by title bar, z-ordering on focus, macOS-style traffic-light close buttons.
Two predefined floats: Camera Feed, Pattern Library (both hidden by default).

---

### 13. Dialogs (`Dialogs.jsx`)

All share `ModalBackdrop` (backdrop blur, click-outside close) + `ModalBox` (white 480px card).

| Dialog | Fields |
|--------|--------|
| **New Project** | Name (required) + Description (optional textarea) |
| **New Task** | Task Name (mono) + Task Type (3-option radio cards) + Save Path (with Browse button) |
| **Add Device** | Device Type (3-col icon cards: Camera/PLC/Robot) + Device Name |
| **Move Device** | Target task picker list |

**Input style**: bg `#252526`, border `#454545`, radius 5, 13px, `Space Grotesk`; focus border `#2b8ce8`

**Primary button**: bg `#2b8ce8` (or device color for Add Device), disabled at 45% opacity

---

### 14. Pattern Manager (`PatternManager.jsx`)

3-pane layout inside the Patterns tab:

```
┌─────────────────────────┬───────────┬─────────────┐
│ Monitor + Result Table  │  Library  │  Properties │
│       (flex: 1)         │  (320px)  │   (from     │
│                         │           │  pm-shared) │
└─────────────────────────┴───────────┴─────────────┘
```

#### Pane 1 — Monitor

**Toolbar** (44px): Trigger, Open Image, Workspace buttons + Active group selector + Show/Use ROI checkboxes + Run Matching button + status chip (IDLE/MATCHING…)

**View tabs**: Raw | Result

**Camera feed**: dark grid bg + corner brackets + center reticle. Shows bounding boxes on Result view.

**KPI strip** (5 cells): FOUND / EXEC TIME / BELOW THRESH / BEST SCORE / PICK POSITION

**Result Table** (resizable via drag handle):
- Column grid: `#` | `Pat #` | Pattern Name | Score | X | Y | Angle | OK
- Score color: ≥0.85 green, ≥0.70 yellow, else red
- Selected row: bg `#2b8ce818`, left border `2px solid #2b8ce8`

#### Pane 2 — Library Tree

Groups as collapsible nodes; patterns as child rows.

**Group row**: warning-yellow `NumBadge` + folder icon + inline-rename name + pattern count + active dot + ⚡ activate + 🗑 delete

**Pattern row**: learned dot (green/red) + output-blue `NumBadge` + inline-rename name + min score + ✏️ edit + 🗑 delete

**Pattern Setting** (below tree):
- Pattern thumbnail placeholder (grid + reticle SVG)
- LEARNED / NOT LEARNED badge
- "Trigger & Learn" + "Open Image" buttons
- Match settings + Edge config (prop rows, toggles)
- Group config: Picking Box W/H/Distance/Angle

#### Pane 3 — Properties

Shared with `pm-shared.jsx`; same collapsible schema UI as PropertyPanel.

---

### 15. Add Pattern Wizard (`PatternWizard.jsx`)

**5-step modal** (920×640px):

| Step | Content |
|------|---------|
| 1. Image | Capture from Camera or Open from File; name + number inputs |
| 2. Crop | Draggable crop rect with corner handles and rule-of-thirds grid |
| 3. Pick Point | Click-to-place crosshair on canvas |
| 4. Picking Box | Symmetric jaw pair with SVG overlay |
| 5. Finish | Preview + summary table |

Validation on step 1 blocks advance until name, number (unique, ≥1), and captured image are all valid.

---

### 16. Edit Pattern Wizard (`EditPatternWizard.jsx`)

**4-step modal** (920×620px) — image locked after learning:

| Step | Content |
|------|---------|
| 1. Identity | Locked image (purple scrim + lock badge) + editable name/number + before value hints |
| 2. Pick Point | Same canvas interaction; pre-filled from saved pickX/pickY |
| 3. Picking Box | Same as Add wizard; pre-filled from saved box config |
| 4. Finish | Diff view: changed fields show `old → new`; unchanged fields not highlighted |

**Lock color**: `#6a5acd` (purple)

**On confirm** updates: name, number, pickX, pickY, pickBoxW, pickBoxH, pickBoxDist, pickBoxAngle on both the pattern and the parent group.

---

## Data Model

```typescript
interface Project {
  id: number;
  name: string;
  description: string;
  tasks: Task[];
}

interface Task {
  id: number;
  name: string;
  type: 'LocalizationTask' | 'PickPlaceTask' | 'InspectTask';
  devices: Device[];
}

interface Device {
  id: string;
  name: string;
  type: 'camera' | 'plc' | 'robot';
  config: Record<string, string>;
  connected: boolean;
}

interface PatternGroup {
  id: string;
  name: string;
  number: number;          // PLC runtime selector
  lowWorkpieceRatio: number;
  pickBoxW: number;        // px
  pickBoxH: number;        // px
  pickBoxDist: number;     // px
  pickBoxAngle: number;    // degrees
  patterns: Pattern[];
}

interface Pattern {
  id: string;
  name: string;
  number: number;          // PLC output ID
  learned: boolean;
  minScore: number;        // 0–1
  angle: number;           // degrees
  tolAngle: number;        // degrees
  maxOverlap: number;      // 0–1
  pickX: number;           // display px
  pickY: number;           // display px
  matchType: 'EdgeBased';
  edge: EdgeMatchConfig;
  boxA?: BoxConfig;
  boxB?: BoxConfig;
}

interface EdgeMatchConfig {
  threshLower: number;
  threshUpper: number;
  kernelSize: 1 | 3 | 5 | 7;
  blurW: number;
  blurH: number;
  greediness: number;
  minReduceLength: number;
  tSamples: number;
  invertBinary: boolean;
  subPixel: boolean;
  stopAtLayer1: boolean;
}

interface BoxConfig {
  w: number;
  h: number;
  dist: number;
  angle: number;
}
```

---

## Navigation Flow

```
App
├── Sidebar (project/task/device tree)
└── DockWorkspace
    ├── TaskTabBar (open tasks)
    ├── TaskWorkspace (active task)
    │   ├── Device Nav (left)
    │   │   ├── Dashboard tab → Dashboard
    │   │   ├── Patterns tab → PatternManagerPanel  (LocalizationTask only)
    │   │   ├── Device rows → VisionPanel / PLCPanel / CameraPanel / RobotPanel
    │   │   └── Settings tab → SettingsPanel
    │   ├── Content area (flex: 1)
    │   └── PropertyPanel (right, 268px)
    ├── BottomDock (System Log | PLC Monitor)
    └── FloatingWindows (Camera Feed, Pattern Library)
```

**Dialogs** rendered at App root level (z-index 1000):
- NewProjectDialog, NewTaskDialog, AddDeviceDialog, MoveDeviceDialog

**Wizards** rendered at PatternManager level (z-index 2000):
- PatternWizard (Add), EditPatternWizard (Edit)

---

## Icon System (`Icons.jsx`)

All icons are inline SVGs (`width={size} height={size}`, `viewBox="0 0 24 24"`, `fill="none"`, `stroke={color}`, strokeWidth 1.6–2.0, `strokeLinecap="round"`).

| Export | Usage |
|--------|-------|
| `IcoDashboard` | Dashboard nav tab |
| `IcoVision` | Vision/camera context |
| `IcoRobot` | Robot device |
| `IcoPLC` | PLC device |
| `IcoCamera` | Camera device |
| `IcoSettings` | Settings tab |
| `IcoFolder` | Project node |
| `IcoTask` | Task node |
| `IcoPlus` | Add actions |
| `IcoChevron` | Expand/collapse (animated rotate) |
| `IcoConnect` | Wifi/connect |
| `IcoNew` | New project |
| `IcoOpen` | Open project |
| `IcoSave` | Save |
| `IcoLog` | System log |
| `IcoRefresh` | Refresh |
| `IcoTrash` | Delete |
| `IcoPattern` | Pattern manager |
| `IcoSignal` | Signal bars |
| `IcoX` | Close/dismiss |
| `IcoCheck` | Checkmark |
| `IcoWarning` | Warning triangle |

---

## Shared Components (`pm-shared.jsx`)

Shared inside the Pattern Manager scope:

| Component | Description |
|-----------|-------------|
| `TBtn` | Ghost button (active/disabled/hover states) |
| `PMIconBtn` | Icon-only button with danger variant |
| `SectionHd` | 26px section header with label + optional count + action |
| `InlineEdit` | Double-click to edit inline text with Enter/Escape |
| `PMModal` | Modal backdrop + card wrapper |
| `AddGroupDialog` | New group name + number with validation |
| `AddPatternDialog` | New pattern name + number with validation |

---

## Files in This Package

| File | Role |
|------|------|
| `NCR Vision Config.html` | App entry point — loads all scripts |
| `components/Icons.jsx` | SVG icon library |
| `components/Sidebar.jsx` | Left project/task/device tree |
| `components/Dialogs.jsx` | New project / task / device / move dialogs |
| `components/Dashboard.jsx` | Task dashboard (stats, charts, log, device status) |
| `components/Panels.jsx` | Vision, PLC, Camera, Settings, Robot panels |
| `components/PropertyPanel.jsx` | Right-side property inspector |
| `components/TaskWorkspace.jsx` | Per-task layout: device nav + content + breadcrumb |
| `components/DockArea.jsx` | Tab bar, bottom dock, floating windows |
| `components/PatternManager.jsx` | 3-pane pattern manager (monitor + library + props) |
| `components/PatternWizard.jsx` | 5-step Add Pattern wizard |
| `components/EditPatternWizard.jsx` | 4-step Edit Pattern wizard |
| `components/pm-shared.jsx` | Shared tokens + atoms for pattern manager |
| `components/App.jsx` | Root state + layout: projects, tasks, devices, routing |

---

## Implementation Notes

1. **Canvas coordinates** — The `PatternWizard` and `EditPatternWizard` work in display pixels (560×380). In production, map these to real image coordinates using the camera calibration scale factor.

2. **Pattern thumbnails** — All pattern image previews are SVG placeholders in the prototype. Replace with actual saved images loaded from disk.

3. **PLC Monitor** simulates live updates (`setInterval` every 1800ms). In production, poll the real PLC via the configured MC Protocol connection.

4. **Inline rename** — Group and pattern names support double-click to rename in-place. The `InlineEdit` component handles keyboard (Enter/Escape) and blur commit.

5. **Resizable panels** — The result table in PatternManager and the bottom dock both use mouse-drag resize handles. Persist panel heights to localStorage for a good UX.

6. **Script loading order** — All components are global window exports, loaded in dependency order in the HTML. In a real bundler (Vite/webpack), convert to ES module imports instead.

7. **The `pm-shared.jsx` vs `PatternManager.jsx` duplication** — Both files define overlapping tokens and components. In production, consolidate into a single shared module.
