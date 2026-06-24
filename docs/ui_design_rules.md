# UI Design Rules — ncr_picking

> **Authority.** This document is the single source of truth for UI structure,
> styling, and theming in this project. It supersedes the UI-specific parts of
> [design_rules.md](design_rules.md) §6 (UI Composition) and §15 (Themed Form
> Stylesheets); those sections now point here. When a UI question is not
> answered by this doc, fall back to `design_rules.md` and then ask.
>
> **Audience.** Anyone adding or changing a widget, form, `.ui`, `.qss`, icon,
> or theme. Read §0–§3 before touching any widget; read §4–§6 before touching
> theming or a custom-painted surface.

---

## 0. First principles

Three rules drive everything below. If a decision is ambiguous, pick the
option that best honours these.

1. **Separation of concerns — structure, style, behaviour are three files.**
   Layout structure lives in `.ui`, visual style lives in `.qss`, behaviour and
   wiring live in `.cpp`. A widget that mixes them (inline stylesheet in cpp,
   `new QPushButton` for a layout primitive, geometry hard-coded in code) is a
   defect, not a style preference.

2. **One theming authority.** `ThemeManager` is the *only* mechanism that
   decides colours and the active theme. Every themable surface reacts to it.
   No widget invents its own light/dark switch.

3. **Tokens over literals.** Colours come from the named palette in §5, never
   from ad-hoc hex literals scattered across `.qss` files.

---

## 1. Layer separation — what goes where

| Concern | Belongs in | Never in |
|---|---|---|
| Widget tree, layouts, spacing, margins, size policies | `.ui` | cpp (no `new QHBoxLayout` for static structure) |
| `objectName` (the styling/anchor handle) | `.ui` | — |
| Colours, fonts, borders, radii, padding, hover/checked states | `.qss` | cpp (`setStyleSheet`), `.ui` (`styleSheet` property) |
| State/role flags that drive variant styling | `.ui` dynamic properties | hard-coded per-state stylesheet strings |
| Signal/slot wiring, data binding, model updates | `.cpp` | — |
| Theme reaction (`reloadStyleSheet`, repolish) | `.cpp` | — |
| Painted (QPainter) surface colours | C++ token source (§6) | `.qss` (QSS cannot style a custom-painted canvas) |

**Rule 1.1 — Layout primitives are authored in `.ui`.** Buttons, labels,
frames, stacks, and their layouts are declared in Qt Designer. Code only wires
them (text, icon, signals, button-group membership). Do not `new` a navigation
button or a static frame in cpp.

**Rule 1.2 — Code-built widgets are the exception, and justified.** Dynamic,
data-driven children (e.g. one row per camera) may be built in code, but the
*container* and its layout still live in the `.ui`, and the built children
still follow the QSS + token rules.

---

## 2. The `.ui` rules

**Rule 2.1 — `objectName` is mandatory on every styled or wired widget.** It is
the QSS selector and the code handle. Use `snake_case` that reflects role and
hierarchy (`btn_nav_dashboard`, `frame_device_header`, `lbl_status_value`).

**Rule 2.2 — The `styleSheet` property must be empty in every `.ui`.** Visual
styling is the `.qss` file's job. An inline `.ui` stylesheet is invisible to a
global theme switch, duplicates the palette, and silently overrides the `.qss`.
This is a hard gate in review.

**Rule 2.3 — Variants are dynamic properties, not separate widgets.** When a
widget needs different looks by state or role (selected card, accent colour,
label role), declare a dynamic property in the `.ui`
(`<property name="selected" stdset="0"><bool>…</bool></property>`) and target it
in QSS via attribute selectors. See §3.4 and §4.4.

**Rule 2.4 — Spacing comes from layouts, not magic numbers in paint code.** Use
layout margins/spacing and size policies. Reserve fixed sizes for genuinely
fixed things (icon buttons), and prefer size policies + minimums otherwise.

**Rule 2.5 — Reusable nav clusters live together in the `.ui`.** Group related
controls (nav bar, toolbar) in one container so a fourth item is a Designer edit,
not a cpp change.

---

## 3. The QSS rules

### 3.1 Two QSS layers — global first

The project has two stylesheet layers. **Prefer the global layer; add a per-form
file only when a widget genuinely needs bespoke styling.**

| Layer | File | Scope | Applied by |
|---|---|---|---|
| **Global** | `resrc/styles/dark.qss`, `resrc/styles/light.qss` | App-wide base look of every standard Qt control | `ThemeManager` via `ThemeStyle::qssPath` |
| **Per-form** | `resrc/styles/<form>_<theme>.qss` | One form/widget with styling the global sheet cannot express | the widget's own `reloadStyleSheet()` (§4.2) |

**Rule 3.1 — Default to the global sheet.** A new widget should look correct
from `dark.qss`/`light.qss` alone. Only introduce a per-form pair when the
widget has styling the global selectors genuinely cannot reach. This keeps the
number of QSS files bounded.

**What goes where:**

| Style concern | Layer | Reason |
|---|---|---|
| Standard Qt controls — `QPushButton`, `QLabel`, `QLineEdit`, `QFrame`, `QScrollArea`, `QSplitter`, `QComboBox`, `QSpinBox`, `QCheckBox`, `QGroupBox`, `QTabWidget` | Global `dark.qss` / `light.qss` | One rule governs every instance app-wide |
| Reusable control variants — `GhostButton`, `PrimaryButton`, `NavButton` (subclasses, §3.6) | Global, selector by class name | Variant identity is the class name, not the form |
| Form-specific `objectName` overrides (e.g. a card grid unique to one screen) | Per-form `<form>_dark.qss` | objectName is local to that form; global rules can't target it |
| Domain-specific dynamic property selectors (e.g. `[connectionState]`, `[deviceColor]`) | Per-form | Selector semantics are meaningful only inside that form |
| Complex structural overrides (nav rail, bespoke status chips, accent panels) | Per-form | Layout context is too specific for a global rule |

**Rule 3.2 — Per-form files are a `<form>_dark.qss` + `<form>_light.qss` pair**,
placed in `resrc/styles/` and registered in [resrc.qrc](../resrc.qrc) with a
`styles/...` alias. Both variants always exist together — never ship dark without
light.

### 3.2 No styling in cpp

**Rule 3.3 — No `setStyleSheet(<literal>)` in cpp.** The only stylesheet code a
widget runs is loading a `.qss` resource in `reloadStyleSheet()` (§4.2).
Inline strings fight the theme switch and force a rebuild for a colour tweak.

### 3.3 Selector conventions

**Rule 3.4 — Style by `objectName`, group related selectors.** One rule per
visual state listing all members
(`QPushButton#btn_a, QPushButton#btn_b { … }` then `…:hover`, `…:checked`), not
one block per widget. Group selectors are how nav clusters stay consistent.

**Rule 3.5 — Remove orphan rules with the widget.** Deleting a widget from a
`.ui` removes its QSS rule in the same change. No dangling selectors.

### 3.4 Variant styling

**Rule 3.6 — Variants use attribute selectors against dynamic properties.**
`QFrame#card[selected="true"] { … }`, `QPushButton#btn[deviceColor="camera"] { … }`.
After mutating the property in code, repolish (§4.4) or the selector reads the
stale value.

### 3.5 Colours

**Rule 3.7 — Reference colours by token, not hex literal.** Every themed colour
is written as a token placeholder `@{group.token}` (e.g. `@{accent.primary}`,
`@{bg.surface}`, `@{state.error}`). `ThemeManager::resolveTokens()` substitutes
each placeholder with the §5 value for the active theme when the sheet is loaded
(global apply and every per-form `reloadStyleSheet()` route through it). The
canonical token → {dark, light} table lives once in
[src/utils/theme_manager.cpp](../src/utils/theme_manager.cpp) (`tokenTable()`),
mirroring §5; a rename is therefore a single-table edit. A reviewer rejects a raw
hex that corresponds to a §5 token — use the `@{token}` form instead. Raw hex is
allowed only for a genuinely off-palette one-off, and then only with a comment
justifying the exception (§5.1). Unknown tokens are left verbatim and logged, so a
typo surfaces rather than rendering a wrong colour.

### 3.6 Reusable control variants via subclassing

Some controls need a different visual role that appears across multiple forms
(ghost button, primary action button, danger button, nav button). These are
styled in the **global** sheet using the class name as the selector — no
per-form QSS file, no property management.

**Rule 3.8 — Reusable variant controls are thin subclasses.**  
A variant class inherits only to acquire a stable class name; it adds no logic.
Inherit all constructors with `using Base::Base`. Place the class in
`src/form/widgets/`.

```cpp
// src/form/widgets/ghost_button.h
#pragma once
#include <QPushButton>

class GhostButton : public QPushButton {
    Q_OBJECT
public:
    using QPushButton::QPushButton;
};
```

**Rule 3.9 — Style the variant by class name in the global sheet.**  
Qt QSS resolves `GhostButton { … }` to any instance of that class or its
subclasses. Write one block per state and keep it in `dark.qss` / `light.qss`.

```qss
/* dark.qss */
GhostButton {
    background: transparent;
    border: 1px solid #3a3f48;   /* border.default */
    color: #e0e4ea;               /* text.primary */
    border-radius: 5px;
    padding: 6px 16px;
}
GhostButton:hover  { background: rgba(232,124,0,0.12); border-color: #e87c00; }
GhostButton:pressed { background: rgba(176,90,0,0.18); }
GhostButton:disabled { color: #4a5260; border-color: #2e3238; }
```

**Rule 3.10 — Promote the widget in Qt Designer.**  
Right-click the button in Designer → *Promote to…* → enter the class name
(`GhostButton`) and header path (`form/widgets/ghost_button.h`). The `.ui`
stays structure-only; no `styleSheet` property is set.

---

## 4. Theming system (`ThemeManager`)

### 4.1 One authority, extensible

`ThemeManager` (singleton, [src/utils/theme_manager.h](../src/utils/theme_manager.h))
owns the active style. `light` and `dark` are pre-registered; additional styles
(e.g. `high_contrast`) are added via `registerStyle(ThemeStyle)`. A `ThemeStyle`
carries an `isDark` flag (drives icon variant), an optional `QPalette`, and a
global `qssPath`.

**Rule 4.1 — Add a theme by registering a `ThemeStyle`, not by branching in
widgets.** Widgets must keep working for any registered style; they switch on
`isDark()`, never on a hard-coded style id.

### 4.2 The `reloadStyleSheet()` contract

A widget that ships a per-form `.qss` pair follows this exact contract:

- In its init, call `reloadStyleSheet()` once, then subscribe to
  `ThemeManager::themeChanged` to call it again on toggle.
- `reloadStyleSheet()` picks the variant from `ThemeManager::instance()->isDark()`,
  loads the resource, and applies it to the widget.

```cpp
// In the constructor / initWidget(), after ui->setupUi(this):
reloadStyleSheet();
connect(ThemeManager::instance(), &ThemeManager::themeChanged,
        this, [this](const QString &, bool) { reloadStyleSheet(); });

// Member:
void MyWidget::reloadStyleSheet() {
    const QString path = ThemeManager::instance()->isDark()
        ? QStringLiteral(":/styles/my_widget_dark.qss")
        : QStringLiteral(":/styles/my_widget_light.qss");
    QFile f(path);
    if (f.open(QFile::ReadOnly | QFile::Text))
        setStyleSheet(QString::fromUtf8(f.readAll()));
}
```

**Rule 4.2 — The method is named `reloadStyleSheet()`.** Do not invent
per-widget names (`applyTheme()`, `restyle()`, …). Uniform naming makes the
pattern greppable and reviewable. (Existing non-conforming widgets are flagged
for migration in [later_todo_list.md](later_todo_list.md).)

**Rule 4.3 — Subscribe on init, and let Qt disconnect on destroy.** The lambda
captures `this`; because the receiver context object is `this`, the connection
dies with the widget. Always pass `this` as the context, never a dangling
context.

### 4.3 Icons follow the theme

**Rule 4.4 — Theme-aware icons go through `ThemeManager::themedIcon()`.**
Icons named `foo.svg` carry light-on-dark paths; `foo_dark.svg` carry
dark-on-light paths. Ask the manager for the right variant; never hard-code one.

### 4.4 Repolish after a property change

**Rule 4.5 — `setProperty()` on a styled widget must be followed by a repolish.**
Qt does not re-evaluate QSS on property change:

```cpp
w->setProperty("selected", true);
w->style()->unpolish(w);
w->style()->polish(w);
w->update();
```

Wrap this in a small `repolish(QWidget*)` helper and call it from every variant
toggle.

---

## 5. Design tokens (colour palette)

These are the **canonical** colours. `.qss` files reference them by the
`@{group.token}` placeholder (Rule 3.7); the values below are the single source
of truth, transcribed into [src/utils/theme_manager.cpp](../src/utils/theme_manager.cpp)
`tokenTable()` and resolved at load time. A `.qss` should not carry a hex that
equals a token's value — use the token. The migration of legacy hardcodes onto
the placeholder form now includes the device-identity, status-bright, deep
accent, and overlay families below. The only accepted raw exception in the
current sweep scope is `#7a1010` (single-use delete-button shadow); document any
future raw exception explicitly when touched and keep the design rationale at
[resrc/styles/THEME_PALETTE_DESIGN_BRIEF.md](../resrc/styles/THEME_PALETTE_DESIGN_BRIEF.md).

### 5.0 Active theme — Hybrid (Graphite Vision + Orange)

**Theme name**: `hybrid`  
**Dark base**: Graphite Vision — cool blue-gray graphite, window `#1a1c20`  
**Accent**: Navy Ops Orange — Siemens industrial orange, `#e87c00`  
**Rationale**: Cool graphite background creates a temperature-contrast pairing
with warm orange accent — a technique standard in industrial HMI (Siemens TIA
Portal, Beckhoff TwinCAT). State colours remain hue-neutral so alarms are
unambiguous regardless of accent.

---

### 5.1 Background tokens

| Token | Role | Dark | Light |
|---|---|---|---|
| `bg.window` | App / page base | `#1a1c20` | `#f2f3f5` |
| `bg.surface` | Panel / sidebar / card | `#22252a` | `#ffffff` |
| `bg.elevated` | Raised containers, dropdowns | `#2c3038` | `#e8eaee` |
| `bg.input` | Inputs, combos, editable fields | `#2c3038` | `#ffffff` |
| `bg.disabled` | Disabled control fill | `#1e2024` | `#eeeff1` |

### 5.2 Border tokens

| Token | Role | Dark | Light |
|---|---|---|---|
| `border.default` | Standard separators, control edges | `#3a3f48` | `#c8ccd4` |
| `border.strong` | Emphasised borders, section dividers | `#4e5562` | `#a8adb8` |
| `border.focus` | Keyboard / active focus ring | `#e87c00` | `#c06400` |
| `border.disabled` | Disabled control border | `#2e3238` | `#dddfe3` |

### 5.3 Text tokens

| Token | Role | Dark | Light |
|---|---|---|---|
| `text.primary` | Primary foreground | `#e0e4ea` | `#1a1c20` |
| `text.muted` | Secondary / label text | `#7a8898` | `#5a6878` |
| `text.disabled` | Disabled text | `#4a5260` | `#a0a8b0` |
| `text.on-accent` | Text rendered on accent-coloured fills | `#ffffff` | `#ffffff` |
| `text.link` | Hyperlink / clickable label | `#ffa040` | `#9a5000` |

### 5.4 Accent tokens

| Token | Role | Dark | Light |
|---|---|---|---|
| `accent.primary` | Primary interactive accent | `#e87c00` | `#c06400` |
| `accent.bright` | Hover / highlight accent | `#ffa040` | `#e08020` |
| `accent.pressed` | Pressed / active deep accent | `#b05a00` | `#8a4400` |
| `accent.pressed.deep` | Extra-deep accent pressed tone | `#7a3c00` | `#5a3800` |
| `accent.subtle` | Translucent accent (hover bg, focus glow) | `rgba(232,124,0,0.12)` | `rgba(192,100,0,0.09)` |
| `selection.bg` | Selected row / item fill | `#3d2e10` | `#fde8c0` |
| `selection.text` | Selected row / item text | `#e0e4ea` | `#1a1c20` |

### 5.5 State tokens

**Confirmed** — no longer proposed. Add to both `dark.qss` and `light.qss`
before shipping the first status-coloured surface.

| Token | Role | Dark | Light |
|---|---|---|---|
| `state.error` | Error / invalid | `#ff5252` | `#c0282a` |
| `state.warning` | Warning / attention | `#ffb020` | `#9a6800` |
| `state.success` | Success / connected / healthy | `#40c870` | `#1a7a40` |
| `state.info` | Informational / neutral status | `#40a8e0` | `#0060a0` |
| `state.error.surface` | Error card / inline alert bg | `rgba(255,82,82,0.10)` | `rgba(192,40,42,0.08)` |
| `state.warning.surface` | Warning card bg | `rgba(255,176,32,0.10)` | `rgba(154,104,0,0.08)` |
| `state.success.surface` | Success card bg | `rgba(64,200,112,0.10)` | `rgba(26,122,64,0.08)` |
| `state.info.surface` | Info card bg | `rgba(64,168,224,0.10)` | `rgba(0,96,160,0.08)` |

> **Rule §2 (WCAG)**: Never use a state colour as the *sole* cue. Always pair
> with an icon or text label. Use `state.*.surface` for large background areas
> — never solid `state.*` colour on anything wider than a 3 px indicator bar.

### 5.6 Accent-tinted panel tokens

For nav rail, sidebar headers, or accent panels that need an orange-warm tint
layered on graphite. Use these before introducing any new warm shade inline.

| Token | Role | Dark | Light |
|---|---|---|---|
| `panel.accent.deep` | Deepest accent panel (nav bg) | `#1e1c16` | `#f5ede0` |
| `panel.accent.mid` | Mid accent panel (sidebar header) | `#252118` | `#faebd0` |
| `panel.accent.surface` | Raised accent surface | `#2e2a1e` | `#f5e4c0` |
| `panel.accent.border` | Border within accent panel | `#403a28` | `#d8c89a` |

### 5.7 Device-identity tokens

Used for device breadcrumbs, nav labels, wizard cards, and tinted device-active
states. These are semantic role colours, not the primary theme accent.

| Token | Role | Dark | Light |
|---|---|---|---|
| `device.camera` | Camera base | `#3a9bd9` | `#1f6fb8` |
| `device.camera.bright` | Camera hover/highlight | `#5cb3e8` | `#3a8ad0` |
| `device.camera.deep` | Camera pressed/deep | `#2278b8` | `#155a98` |
| `device.camera.tint.bg` | Camera tinted fill | `rgba(58,155,217,0.12)` | `rgba(31,111,184,0.08)` |
| `device.camera.tint.bd` | Camera tinted border | `rgba(58,155,217,0.40)` | `rgba(31,111,184,0.36)` |
| `device.plc` | PLC base | `#3ac98a` | `#1a8a55` |
| `device.plc.bright` | PLC hover/highlight | `#5cdca2` | `#28a468` |
| `device.plc.deep` | PLC pressed/deep | `#22a070` | `#0e6a40` |
| `device.plc.tint.bg` | PLC tinted fill | `rgba(58,201,138,0.12)` | `rgba(26,138,85,0.08)` |
| `device.plc.tint.bd` | PLC tinted border | `rgba(58,201,138,0.40)` | `rgba(26,138,85,0.36)` |
| `device.output` | Vision-output base | `#a87de0` | `#7a4ec0` |
| `device.output.bright` | Vision-output hover/highlight | `#bf98ea` | `#9168d0` |
| `device.output.deep` | Vision-output pressed/deep | `#8458c0` | `#5e389a` |
| `device.output.tint.bg` | Vision-output tinted fill | `rgba(168,125,224,0.12)` | `rgba(122,78,192,0.08)` |
| `device.output.tint.bd` | Vision-output tinted border | `rgba(168,125,224,0.40)` | `rgba(122,78,192,0.36)` |
| `device.robot` | Robot base | `#e0b020` | `#a07000` |
| `device.robot.bright` | Robot hover/highlight | `#f0c850` | `#b88810` |
| `device.robot.deep` | Robot pressed/deep | `#b88a10` | `#7a5400` |
| `device.robot.tint.bg` | Robot tinted fill | `rgba(224,176,32,0.12)` | `rgba(160,112,0,0.08)` |
| `device.robot.tint.bd` | Robot tinted border | `rgba(224,176,32,0.40)` | `rgba(160,112,0,0.36)` |
| `device.default` | Fallback / unknown base | `#7a8ca8` | `#4a5868` |
| `device.default.bright` | Fallback hover/highlight | `#94a4be` | `#5e6c80` |
| `device.default.deep` | Fallback pressed/deep | `#5a6c88` | `#36404e` |
| `device.default.tint.bg` | Fallback tinted fill | `rgba(122,140,168,0.12)` | `rgba(74,88,104,0.08)` |
| `device.default.tint.bd` | Fallback tinted border | `rgba(122,140,168,0.40)` | `rgba(74,88,104,0.36)` |

### 5.8 State bright/deep helper tokens

These complement §5.5 when a control needs a lighter gradient stop or a deeper
danger pressed colour.

| Token | Role | Dark | Light |
|---|---|---|---|
| `state.success.bright` | Brighter success stop | `#2eff98` | `#44c878` |
| `state.error.bright` | Brighter error stop | `#ff8a8a` | `#e84848` |
| `state.warning.bright` | Brighter warning stop | `#ffd060` | `#d09020` |
| `state.error.deep` | Danger pressed/deep | `#922015` | `#6e1010` |

### 5.9 Overlay tokens

Theme-agnostic overlay helpers for dock-button hover/pressed states, scrims,
and elevation tints.

| Token | Role | Dark | Light |
|---|---|---|---|
| `overlay.scrim.weak` | Weak black scrim | `rgba(0,0,0,0.08)` | `rgba(0,0,0,0.08)` |
| `overlay.scrim.med` | Medium black scrim | `rgba(0,0,0,0.16)` | `rgba(0,0,0,0.16)` |
| `overlay.scrim.strong` | Strong black scrim | `rgba(0,0,0,0.32)` | `rgba(0,0,0,0.32)` |
| `overlay.tint.weak` | Weak white tint | `rgba(255,255,255,0.04)` | `rgba(255,255,255,0.04)` |
| `overlay.tint.med` | Medium white tint | `rgba(255,255,255,0.10)` | `rgba(255,255,255,0.10)` |
| `overlay.tint.strong` | Strong white tint | `rgba(255,255,255,0.24)` | `rgba(255,255,255,0.24)` |

### 5.α Canonical tint-alpha scale

Alpha-only guidance for new tinted families; prefer these levels before
inventing a fresh translucent literal.

| Level | Dark α | Light α | Use |
|---|---|---|---|
| `tint.alpha.subtle` | 0.10 | 0.08 | resting tinted fill |
| `tint.alpha.hover` | 0.18 | 0.14 | pointer-hover on tinted surface |
| `tint.alpha.active` | 0.28 | 0.22 | selected / pressed tinted fill |
| `tint.alpha.border` | 0.40 | 0.36 | outline of a tinted surface |

---

**Rule 5.1 — A colour used twice is a token.** The second time a hex appears,
promote it to §5 and reference the token. A single-use decoration may stay
inline only with a comment justifying the exception.

**Rule 5.2 — Renaming or re-valuing a token is a single-file operation.** Update
§5 and the matching entry in `theme_manager.cpp` `tokenTable()`. `.qss` files
reference the token by name (`@{token}`), so no `.qss` edit is needed — the new
value resolves everywhere on the next load. (Contrast the old convention, where
the hex was cached in every `.qss` and a rename meant a project-wide hex search.)

---

## 6. Non-QSS theming — custom-painted surfaces

QSS cannot style a `QPainter`-rendered canvas (e.g. the pattern canvas). Those
surfaces read their colours from a C++ token source.

**Rule 6.1 — Painted surfaces pull colours from a theme token header, keyed off
`ThemeManager::isDark()`**, and repaint on `themeChanged`. See
[src/form/pattern/pattern_theme.h](../src/form/pattern/pattern_theme.h) for the
existing pattern. New painted surfaces mirror it; they do not embed raw
`QColor(0x…)` literals in `paintEvent`.

**Rule 6.2 — Keep the C++ token names aligned with §5.** A painted "accent"
should equal `accent.primary` for the active theme, so painted and QSS surfaces
match.

---

## 7. Standard themed-widget skeleton

Copy this when adding a widget that needs a per-form theme. It encodes every
rule above.

```cpp
// my_widget.cpp
MyWidget::MyWidget(QWidget *parent) : QWidget(parent), ui(new Ui::MyWidget) {
    ui->setupUi(this);            // structure from my_widget.ui (Rule 1.1, 2.x)

    // ... signal/slot wiring (behaviour) ...

    reloadStyleSheet();           // Rule 4.2
    connect(ThemeManager::instance(), &ThemeManager::themeChanged,
            this, [this](const QString &, bool) { reloadStyleSheet(); });
}

void MyWidget::reloadStyleSheet() {
    const QString path = ThemeManager::instance()->isDark()
        ? QStringLiteral(":/styles/my_widget_dark.qss")
        : QStringLiteral(":/styles/my_widget_light.qss");
    QFile f(path);
    if (f.open(QFile::ReadOnly | QFile::Text))
        setStyleSheet(QString::fromUtf8(f.readAll()));
}

// Variant toggle (Rule 2.3, 3.6, 4.5):
void MyWidget::setSelected(QWidget *card, bool on) {
    card->setProperty("selected", on);
    card->style()->unpolish(card);
    card->style()->polish(card);
    card->update();
}
```

Checklist for the matching files:
- `my_widget.ui` — structure only, every styled widget has an `objectName`,
  `styleSheet` property empty, variants as dynamic properties.
- `resrc/styles/my_widget_dark.qss` + `_light.qss` — registered in `resrc.qrc`,
  colours are §5 tokens, selectors grouped by state.

---

## 8. Naming conventions

| Artifact | Convention | Example |
|---|---|---|
| `objectName` | `snake_case`, role-first | `btn_nav_settings`, `lbl_device_name` |
| Per-form QSS files | `<form>_<theme>.qss` | `signals_monitor_widget_dark.qss` |
| QSS resource alias | `styles/<file>` | `:/styles/signals_monitor_widget_dark.qss` |
| Dynamic property | `lowerCamel` flag/role | `selected`, `deviceColor`, `labelRole` |
| Theme reload method | exactly `reloadStyleSheet()` | — |
| Token name | `group.semantic` (this doc) | `accent.primary`, `bg.surface` |
| Variant widget subclass | `PascalCase`, role-based | `GhostButton`, `PrimaryButton`, `NavButton`, `DangerButton` |
| Variant widget source location | `src/form/widgets/<name>.h` + `.cpp` | `src/form/widgets/ghost_button.h` |

---

## 9. Review checklist (PR gate for UI changes)

A UI change is not ready to merge unless all of these hold:

- [ ] Structure is in `.ui`; no layout primitives `new`-ed in cpp.
- [ ] Every styled/wired widget has a meaningful `objectName`.
- [ ] No `styleSheet` property set in any `.ui`.
- [ ] No `setStyleSheet(<literal>)` in cpp — only resource loads in
      `reloadStyleSheet()`.
- [ ] If a per-form `.qss` was added, both `_dark` and `_light` exist and are
      registered in `resrc.qrc`.
- [ ] All colours are §5 tokens; no new raw hex (or a justifying comment).
- [ ] Theme reaction uses `reloadStyleSheet()` + `themeChanged` subscription
      with `this` as context.
- [ ] Variant styling uses dynamic properties + attribute selectors, with a
      repolish after every `setProperty`.
- [ ] Theme-aware icons go through `themedIcon()`.
- [ ] Custom-painted surfaces read colours from a theme token header, not raw
      `QColor` literals, and repaint on `themeChanged`.
- [ ] Reusable button/control variants are thin subclasses in `src/form/widgets/`,
      styled by class name in the global sheet — no per-form QSS added just for
      a button style.
- [ ] Widget renders correctly in **both** dark and light before review.

---

## 10. Relationship to other docs

- [design_rules.md](design_rules.md) §6, §15 — superseded by this doc for
  UI/QSS; kept as short pointers.
- [later_todo_list.md](later_todo_list.md) — follow-up UI conformance work that
  is orthogonal to the token table itself, for example the remaining
  theme-aware icon audit.
- The `qt-ui-design` agent skill may be used to audit a screen against these
  rules.

---

## 11. Changing the colour scheme — where to touch

When switching to a new colour palette (e.g. replacing Graphite Vision + Orange
with another theme family), the following locations **must all be updated
together**. Touching only one layer will leave visible mismatches.

### 11.1 The four mandatory locations

**1 — §5 token table (this doc)**  
Update first. Every other location derives from §5. After editing the table
here, do a hex-search across the QSS files to find cached copies that are now
stale (Rule 5.2).

**2 — `src/utils/theme_manager.cpp` — `buildDarkPalette()` / `buildLightPalette()`**  
The QPalette and the QSS token palette must stay in sync. The palette is the
fallback colour source for any widget not explicitly covered by QSS, and it is
the colour source for `palette()` references inside the ADS docking stylesheet
(`focus_highlighting.css`). See §11.2 for the role → token mapping.

**2b — `src/utils/theme_manager.cpp` — `tokenTable()`**  
The canonical token → {dark, light} values consumed by `resolveTokens()`. Any
colour referenced in QSS as `@{token}` updates everywhere from this one table —
no `.qss` edit needed. Keep it identical to §5.

**3 — `resrc/styles/dark.qss` + `light.qss` — global stylesheet**  
Every standard Qt control and every `ads--` ADS selector lives here. Token-backed
colours are written `@{token}` and re-value automatically via 2b; only the
residual **raw** hex (off-palette one-offs, ADS `ads--` literals not yet
tokenized) needs a manual search-and-replace. The ADS section is at the end of
both files; keep it consistent with the palette (§11.2).

**4 — Per-form QSS pairs (`resrc/styles/<widget>_dark.qss` + `_light.qss`)**  
Each widget with bespoke styling has its own QSS pair. Token-backed colours
update via 2b; replace only residual raw hex. The `LocalizationTaskWidget`
nav-panel `panel.accent.*` tokens re-value from the table when the accent
changes (§5.6).

### 11.2 QPalette role → §5 token mapping

This mapping is the contract. When §5 changes, `buildDarkPalette()` and
`buildLightPalette()` must be updated to reflect it.

| QPalette role | §5 token | Why it matters |
|---|---|---|
| `Window` | `bg.window` | Default fill for QWidget / QMainWindow / content areas |
| `Base` | `bg.window` (dark) / `bg.surface` (light) | Input field base background |
| `Button` | `bg.elevated` | Fusion style button fill |
| `AlternateBase` | `bg.elevated` | Alternating rows in table/tree views |
| `Light` | `bg.surface` | **ADS `focus_highlighting.css` uses `palette(light)` for `ads--CDockWidget` background** |
| `Midlight` | `bg.elevated` | Fusion style intermediate tones |
| `Dark` | `border.default` | **ADS `focus_highlighting.css` uses `palette(dark)` for splitter and sidebar borders** |
| `Mid` | `border.default` | Fusion style separator rendering |
| `WindowText` / `Text` / `ButtonText` | `text.primary` | Default text colour for all widgets |
| `Highlight` | `accent.primary` | Selection highlight colour throughout the app |
| `HighlightedText` | `text.on-accent` (`#ffffff`) | Text on accent-coloured selection |
| `ToolTipBase` | `bg.surface` | Tooltip background |
| `ToolTipText` | `text.primary` | Tooltip text |
| `Disabled / WindowText` | `text.disabled` | Disabled control text |
| `Shadow` | (leave near-black / near-white) | Drop shadows — rarely visible |

> **ADS note** — Qt Advanced Docking System loads `focus_highlighting.css`
> internally via `CDockManager::setStyleSheet()` (widget-level, higher priority
> than `qApp->setStyleSheet()`). To let the global QSS own all ADS colours,
> call `m_dockManager->setStyleSheet(QString())` immediately after constructing
> the CDockManager. This clears ADS's widget-level stylesheet and lets
> `ads--` rules in `dark.qss` / `light.qss` take effect. Even so, `palette()`
> references are still resolved from the active QPalette — so §11.2 must be
> correct or ADS widget backgrounds will not match the §5 tokens.

### 11.3 Change-order checklist

Follow this order to avoid transient mismatches:

1. Update §5 token table in this doc.
2. Update `tokenTable()` in `theme_manager.cpp` to match §5 (this re-values every
   `@{token}` reference across all sheets at once).
3. Update `buildDarkPalette()` + `buildLightPalette()` in `theme_manager.cpp`
   per the role → token mapping above.
4. Replace only the residual **raw** hex in `dark.qss` + `light.qss` and the
   per-form pairs (off-palette one-offs and not-yet-tokenized ADS literals);
   `@{token}` colours are already handled by step 2.
5. If the **accent** colour changed: re-derive and update the `panel.accent.*`
   tokens in §5.6 and the `LocalizationTaskWidget` nav-panel QSS sections.
6. If the **accent** colour changed: update `src/form/pattern/pattern_theme.h`
   and any other C++ painted-surface token headers (§6).
7. Build and test both dark and light in the running app — QSS cannot be
   validated by type-checker or test suite; visual inspection is mandatory.
