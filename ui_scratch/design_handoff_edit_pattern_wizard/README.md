# Handoff: Edit Pattern Wizard

## Overview

A 4-step modal wizard that lets operators edit an existing learned pattern in the NCR Vision Config application. Unlike the **Add Pattern Wizard** (5 steps), the image is already saved to disk after learning, so the Image and Crop steps are removed. The wizard allows updating:

- Pattern **name** and **number**
- **Picking position** (robot TCP offset relative to the pattern)
- **Picking box** geometry (symmetric gripper jaw bounds)

## About the Design Files

The files in this bundle are **HTML/JSX design references** — high-fidelity prototypes showing the intended look and behavior. They are **not** production code to ship directly. The task is to **recreate these designs in the target codebase's existing environment** (the NCR Vision Config React app) using its established component patterns, state management, and design tokens. All component logic should be adapted to the real data layer and existing component library.

## Fidelity

**High-fidelity.** Colors, typography, spacing, component shapes, hover states, validation logic, and canvas interactions are all finalised. Recreate pixel-accurately using the codebase's existing libraries and patterns.

---

## Feature: Edit Pattern Wizard

### Trigger

In the **Pattern Library** tree pane (`LibraryPane`), each pattern row now exposes two action icon buttons on hover:

| Button | Icon | Action |
|--------|------|--------|
| Edit   | Pencil (✏️) | Opens `EditPatternWizard` for this pattern |
| Delete | Trash 🗑️ | Deletes pattern (existing behaviour) |

The edit button calls `onEditPattern(groupId, patternId)` which sets dialog state `{ type: 'editPattern', groupId, patternId }`.

---

## Modal Shell

| Property | Value |
|----------|-------|
| Width | 920 px |
| Height | 620 px |
| Background | `#2d2d2d` |
| Border | `1px solid #454545` |
| Border radius | 9 px |
| Box shadow | `0 24px 64px rgba(0,0,0,0.6)` |
| Backdrop | `rgba(5,9,18,0.78)` + `backdrop-filter: blur(3px)` |
| z-index | 2000 |
| Layout | `flex-column` — header / step-rail / body / footer |

### Header (44 px tall)

- Background: `#1e1e1e`, bottom border `1px solid #3c3c3c`
- Left: icon badge (28×28, `#0e639c22` bg, `#0e639c55` border, radius 5) with pencil SVG in `#0e639c`
- Title: `"Edit Pattern"` — 13 px, weight 700, `#cccccc`
- Subtitle: `"Group: {groupName} · Pattern: {pattern.name} · Step {N} of 4 — {stepSub}"` — 10 px, `#6a6a6a`
- Right: ✕ close button — 18 px, `#9d9d9d`, transparent background

### Step Rail (42 px tall)

- Background: `#1e1e1e`, bottom border `1px solid #3c3c3c`
- 4 equal-width step tabs, `padding: 10px 8px`
- Each tab: step circle (22×22, radius 50%) + label stack
  - **Completed step**: circle bg `#4ec9b0`, white `✓`, label color `#9d9d9d`
  - **Current step**: circle bg `#0e639c`, white number, bottom border `2px solid #0e639c`, label color `#cccccc`
  - **Upcoming step**: circle bg `#252526`, border `1px solid #454545`, number `#6a6a6a`, label `#6a6a6a`
- Clicking a completed/current/adjacent step navigates to it

| # | id | Label | Subtitle |
|---|-----|-------|----------|
| 1 | identity | Identity | Rename or renumber |
| 2 | pick | Pick Point | Reposition picking offset |
| 3 | box | Picking Box | Adjust gripper bounds |
| 4 | finish | Finish | Review & save |

### Body (flex: 1, padding: 18 px, overflow: hidden)

Step content rendered here. See per-step specs below.

### Footer (48 px tall)

- Background: `#1e1e1e`, top border `1px solid #3c3c3c`
- Left: status hint text — 10 px, `#6a6a6a`, `JetBrains Mono`
- Right: button row — `Cancel` | `← Back` | `Next →` / `✓ Save Changes`

**Button styles:**

| Variant | Background | Border | Text | Padding |
|---------|-----------|--------|------|---------|
| Ghost (Cancel / Back) | transparent | `1px solid #3c3c3c` | `#cccccc` | `5px 12px` |
| Primary (Next / Save) | `#0e639c` | transparent | `#ffffff` | `7px 18px` |
| Disabled | `#252526` | `#3c3c3c` | `#6a6a6a` | — |

Hover: primary → `#1177bb`; ghost → bg `#2d2d2d`, border `#454545`

---

## Step 1 — Identity

**Layout:** two columns, `gap: 18px`, full height

### Left column (flex: 1) — Locked image preview

- Background: `#181818`, border `1px solid #3c3c3c`, radius 5, `overflow: hidden`
- Shows the **saved pattern image** (read-only, cannot be changed)
- Full-area **dark scrim** overlay: `rgba(0,0,0,0.38)` — signals locked state
- **Lock badge** (top-left, absolute):
  - Background: `rgba(106,90,205,0.8)` (`#6a5acd` at ~80% opacity)
  - Border: `1px solid #6a5acd`, radius 4, padding `4px 9px`
  - Lock SVG icon (11×11, white) + text `"IMAGE LOCKED"` — 9 px, weight 700, white, `JetBrains Mono`
- **Saved tag** (bottom-left): `"● SAVED · 2448 × 2048"` — 9 px, `#4ec9b0`, `#000a` bg, `1px solid #4ec9b055` border, radius 3
- **Replace note** (bottom-right): `"To replace image, delete & re-add pattern"` — 9 px, `#6a6a6a`, `#000a` bg

### Right column (280 px)

**Notice banner**
- Background: `#6a5acd18`, border `1px solid #6a5acd44`, radius 5, padding `9px 12px`
- Lock icon (13×13, `#6a5acd`) + body text explaining image is locked after learning

**Pattern Name field**
- Label: `"PATTERN NAME"` — 9 px, weight 700, `#6a6a6a`, uppercase, letterspacing `0.08em`
- Input: `width: 100%`, bg `#1e1e1e`, border `1px solid #454545` (error: `#f48771`), radius 4, padding `6px 9px`, 12 px, `Space Grotesk`
- Focus border: `#0e639c`
- If value ≠ original: show `"← was: {origName}"` in `#dcb67a`, 10 px, `JetBrains Mono`
- Error: show message in `#f48771`, 10 px

**Pattern Number field**
- Same styling as name field, `type="number"`
- Hint: `"output register on match"` — 9 px, `#6a6a6a`
- If value ≠ original: show `"← was: #{origNum}"`

**Divider**: `1px solid #3c3c3c`

**Current Identity chip**
- Background: `#1e1e1e`, border `1px solid #3c3c3c`, radius 5, padding `9px 12px`
- Two rows: Name (value `#cccccc`) and Number (value `#9cdcfe`, weight 700)

**Validation** (step advance blocked until):
- Name is non-empty and not duplicate in group
- Number is ≥ 1 and not duplicate in group

---

## Step 2 — Pick Point

**Layout:** two columns, `gap: 18px`, full height

### Left column (flex: 1) — Interactive canvas

- Background: `#181818`, border `1px solid #3c3c3c`, radius 5
- `cursor: crosshair` — clicking sets pick position `{x, y}` in canvas display pixels
- **Horizontal crosshair line**: full width, `top: pick.y`, height 1, `rgba(220,182,122,0.8)` (`#dcb67a`)
- **Vertical crosshair line**: full height, `left: pick.x`, width 1, same color
- **Pick circle**: 18×18, radius 50%, centered on pick point, border `1.5px solid #dcb67a`, bg `#dcb67a22`
- **Coord label** (12px right, 22px above pick): `"PICK (x, y)"` — 9 px, `#dcb67a`, weight 700, `JetBrains Mono`, bg `#000a`, padding `2px 5px`, radius 2
- Bottom hint: `"Click anywhere to reposition the picking point"` — 9 px, `#6a6a6a`

### Right column (280 px)

**Coordinate inputs** (grid 2-col: label + input)
- `X` label: 11 px, `#0e639c`, weight 700, `JetBrains Mono`
- `Y` label: same
- Inputs: `EWNum` spinner — bg `#1e1e1e`, border `1px solid #454545`, radius 4; arrow +/− buttons on right

**Quick-action buttons**: `Center` | `Snap to Boss`

**Info card**: bg `#1e1e1e`, border `1px solid #3c3c3c`, radius 5, padding `10px 12px`
- Heading: 10 px, weight 700, `#6a6a6a`, uppercase
- Body: 11 px, `#9d9d9d`, line-height 1.5 — explains pick offset usage

---

## Step 3 — Picking Box

**Layout:** two columns, `gap: 18px`, full height

### Left column — Canvas with SVG overlay

- Same image as other steps
- SVG overlaid at `position: absolute, inset: 0`:

  **BoxOverlay** component (rendered twice — jaw A and jaw B):
  - Dashed connector line from pick → box centre: `stroke: #4ec9b0`, `stroke-dasharray: 3 3`, opacity 0.55
  - Filled + stroked rotated rect: fill `#4ec9b022`, stroke `#4ec9b0`, strokeWidth 1.5
  - Centre dot: radius 2.5, fill `#4ec9b0`
  - Label text (`"A"` / `"B"`): 9 px, `#4ec9b0`, weight 700, centred above box

  **Box geometry:**
  - Box centre = `pick + (cos(angle)×dist, sin(angle)×dist)`
  - Box A angle = `boxCfg.angle`
  - Box B angle = `boxCfg.angle + 180°` (symmetric mirror)
  - Both jaws share `w`, `h`, `dist`

  **Pick crosshair** (white, 1 px):
  - Horizontal: `pick.x ± 12`
  - Vertical: `pick.y ± 12`
  - Centre dot: radius 3, fill white

- Bottom hint: `"Symmetric gripper jaws — same size, mirrored about the pick point"` — 9 px, `#6a6a6a`

### Right column (280 px)

**Info banner**: bg `#0e639c1a`, border `1px solid #0e639c55`, radius 5 — explains A/B angle relationship

**Box Size inputs** (shared):
- Width, Height — `EWNum` spinners, suffix `"px"`, min 5

**Offset inputs**:
- Distance — `EWNum`, suffix `"px"`, min 0, max 400
- Angle — `EWNum`, suffix `"°"`, min −180, max 180

**Buttons**: `Reset` (restores defaults 120×80, d=90, angle=0) | `Rotate +90°`

**Footnote**: 10 px, `#6a6a6a`, line-height 1.5 — collision rejection explanation

---

## Step 4 — Finish / Review

**Layout:** two columns, `gap: 18px`, full height

### Left column — Final preview

- Image with box + pick overlays (same as Step 3)
- Pick dot: radius 3, fill `#dcb67a` (warm yellow — distinct from Step 3 white)
- **Top-left badge**: `"● CHANGES READY TO SAVE"` — 9 px, weight 700, `#4ec9b0`, `#000a` bg, green border
- **Top-right badge**: lock badge in `#6a5acd` — `"IMAGE UNCHANGED"` — 8 px, weight 700, white

### Right column (280 px, overflow-y: auto)

**Heading row**: pattern name (13 px, weight 700, `#cccccc`) + `#number` (`#0e639c`, `JetBrains Mono`)

**Changes block** (shown only if ≥ 1 field changed):
- Section label: `"{N} Change(s)"` — 9 px, weight 700, `#dcb67a`, uppercase

  **DiffRow** per changed field:
  - 4-column grid: `80px 1fr 16px 1fr`
  - Field name: 9 px, `#6a6a6a`, uppercase
  - Old value: 10 px, `#6a6a6a`, `JetBrains Mono`, `text-decoration: line-through`, opacity 0.7
  - Arrow `→`: 11 px, `#6a6a6a`, centred
  - New value: 10 px, `#4ec9b0`, `JetBrains Mono`, weight 700

  Tracked fields: Name, Number, Pick Point, Box Size, Box Distance, Box Angle

**No-changes state**: grey card — `"No changes made — values are identical to the saved pattern."`

**Final Values summary** (always visible):
- `StaticRow` per field: label (`#6a6a6a`) + value (`#9d9d9d`, `JetBrains Mono`)
- Fields: Name, Number, Pick Point, Box Size, Offset, Image (shows "Locked · saved")

**Confirmation card**: bg `#4ec9b015`, border `#4ec9b055` — explains no retraining needed

---

## State Shape

```typescript
// Props passed into EditPatternWizard
interface EditPatternWizardProps {
  groupName: string;
  pattern: {
    id: string;
    name: string;
    number: number;
    pickX: number;        // display-px pick position X
    pickY: number;        // display-px pick position Y
    pickBoxW: number;     // jaw width (px)
    pickBoxH: number;     // jaw height (px)
    pickBoxDist: number;  // jaw offset distance from pick (px)
    pickBoxAngle: number; // jaw angle (degrees), jaw B = angle + 180°
  };
  usedNums: number[];     // other patterns' numbers (exclude current pattern)
  usedNames: string[];    // other patterns' names (exclude current pattern)
  onConfirm: (updates: PatternUpdates) => void;
  onClose: () => void;
}

// Payload emitted by onConfirm
interface PatternUpdates {
  name: string;
  number: number;
  pickX: number;
  pickY: number;
  pickBoxW: number;
  pickBoxH: number;
  pickBoxDist: number;
  pickBoxAngle: number;
}
```

### Internal wizard state

```typescript
const [step, setStep]       = useState(0);          // 0–3
const [name, setName]       = useState(pattern.name);
const [num,  setNum]        = useState(String(pattern.number));
const [pick, setPick]       = useState({ x: pattern.pickX, y: pattern.pickY });
const [boxCfg, setBoxCfg]   = useState({
  w:     pattern.pickBoxW,
  h:     pattern.pickBoxH,
  dist:  pattern.pickBoxDist,
  angle: pattern.pickBoxAngle,
});
```

---

## Integration in PatternManager

### 1. Dialog state added

```typescript
type DialogState =
  | { type: 'addGroup' }
  | { type: 'addPattern'; groupId: string }
  | { type: 'editPattern'; groupId: string; patternId: string }   // NEW
  | null;
```

### 2. editPatternFromWizard handler

```typescript
const editPatternFromWizard = (gId: string, pId: string, updates: PatternUpdates) => {
  setGroups(prev => prev.map(g => g.id === gId ? {
    ...g,
    // sync group-level box settings
    pickBoxW:     updates.pickBoxW,
    pickBoxH:     updates.pickBoxH,
    pickBoxDist:  updates.pickBoxDist,
    pickBoxAngle: updates.pickBoxAngle,
    patterns: g.patterns.map(p => p.id === pId ? {
      ...p,
      name:   updates.name,
      number: updates.number,
      pickX:  updates.pickX,
      pickY:  updates.pickY,
      boxA: { w: updates.pickBoxW, h: updates.pickBoxH, dist: updates.pickBoxDist, angle: updates.pickBoxAngle },
      boxB: { w: updates.pickBoxW, h: updates.pickBoxH, dist: updates.pickBoxDist, angle: updates.pickBoxAngle + 180 },
    } : p),
  } : g));
  setSel({ groupId: gId, patternId: pId });
  setDlg(null);
};
```

### 3. Resolving pattern data to pass into wizard

```typescript
const patternForWizard = {
  ...dlgPat,
  pickBoxW:    dlgPat.boxA?.w     ?? dlgGrp.pickBoxW,
  pickBoxH:    dlgPat.boxA?.h     ?? dlgGrp.pickBoxH,
  pickBoxDist: dlgPat.boxA?.dist  ?? dlgGrp.pickBoxDist,
  pickBoxAngle:dlgPat.boxA?.angle ?? dlgGrp.pickBoxAngle,
};
```

### 4. Edit button in LibraryPane pattern row

Add a pencil `IBtn` before the existing delete `IBtn`:

```tsx
<IBtn
  title="Edit pattern"
  icon={<PencilIcon />}
  onClick={() => onEditPattern(group.id, pat.id)}
/>
```

---

## Design Tokens

| Token | Value |
|-------|-------|
| Background | `#1e1e1e` |
| Surface | `#252526` |
| Surface elevated | `#2d2d2d` |
| Border subtle | `#3c3c3c` |
| Border default | `#454545` |
| Text primary | `#cccccc` |
| Text secondary | `#9d9d9d` |
| Text muted | `#6a6a6a` |
| Accent blue | `#0e639c` |
| Accent blue hover | `#1177bb` |
| Accent teal (ok) | `#4ec9b0` |
| Accent yellow (warn) | `#dcb67a` |
| Accent red (error) | `#f48771` |
| Accent purple (lock) | `#6a5acd` |
| Output ID blue | `#9cdcfe` |
| Font sans | `Space Grotesk, sans-serif` |
| Font mono | `JetBrains Mono, monospace` |

---

## Files in This Package

| File | Purpose |
|------|---------|
| `NCR Vision Config.html` | Main app entry — includes all script tags |
| `components/EditPatternWizard.jsx` | **New** — the 4-step edit wizard component |
| `components/PatternWizard.jsx` | Reference — the original 5-step add wizard |
| `components/PatternManager.jsx` | Modified — added Edit button + dialog wiring |

---

## Notes for Developer

- The canvas dimensions used in the prototype are **display px** (560×380). In production these coordinates must be mapped to/from actual image pixel coordinates using the camera calibration scale factor.
- The `PartImage` SVG in the prototype is a **placeholder** — replace with the actual saved pattern thumbnail loaded from disk.
- The `BoxOverlay` SVG component can be reused as-is across the Add and Edit wizards since the geometry is identical.
- Step navigation is **forward-only** unless previous steps are already complete; users can jump back freely.
