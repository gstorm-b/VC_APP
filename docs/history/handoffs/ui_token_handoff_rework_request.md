# UI Token Handoff Rework Request

**Date:** 2026-06-24  
**Owner role:** PM / tech lead review  
**Status:** Needs rework before marking the handoff complete

## Context

The design handoff lives in:

- `build/theme_design_context/handoff_from_claude_design/README.md`
- `build/theme_design_context/handoff_from_claude_design/tokens.theme_manager.cpp.txt`
- `build/theme_design_context/handoff_from_claude_design/qss_sweep_map.md`
- `build/theme_design_context/handoff_from_claude_design/tokens.ui_design_rules.md`

The latest implementation made good progress:

- `ThemeManager::tokenTable()` now includes `device.*`, `state.*.bright`,
  `state.error.deep`, and `overlay.*` tokens.
- `docs/rules/ui_design_rules.md` now documents those token families.
- Many QSS selectors now use the new `@{token}` placeholders.
- A token sanity pass found no missing token definitions in actual QSS files.
- Existing `architecture_contract_test.exe -silent` artifact exits with code `0`.

However, the implementation is not yet acceptance-clean against the handoff.

## Previous Findings Now Resolved

As of the 2026-06-24 follow-up review, the residual check for the previous raw
QSS colours and `ProjectTreeWidget` chip colours returns no matches. Keep the
details below as regression context; they should not reopen unless the colours
come back.

### 1. [Resolved] Raw colours from the first handoff review

The handoff acceptance says every raw colour in the sweep scope must be covered
or kept raw with justification, and names only `#7a1010` as the intended raw
exception.

The first handoff review found these raw colours:

- `resrc/styles/dark.qss`
  - `#f0c0c0`
  - `#e0d0a0`
- `resrc/styles/light.qss`
  - `#5a3800`

The sweep map explicitly says:

- `#5a3800` -> `@{accent.pressed.deep}` (new token)
- `#f0c0c0` -> `@{state.error.surface}`
- `#e0d0a0` -> `@{state.warning.surface}`

At that time, inline comments justified those colours as one-offs, which changed
the handoff acceptance instead of implementing it. The original rework options
were:

1. Follow the sweep map and tokenize them, including adding
   `accent.pressed.deep` if needed; or
2. Update the handoff/docs explicitly with a PM-approved decision that these are
   additional accepted raw exceptions.

Resolved expectation: option 1 was applied; keep the residual check below as a
regression guard.

### 2. [Resolved] Custom-painted ProjectTree chips used old raw device colours

`src/widgets/project_tree_widget.cpp` previously hardcoded device/task chip
colours:

- `#2b8ce8`
- `#22d17a`
- `#f5a623`
- `#6b7ea0`

This widget paints chips with `QPainter`, so QSS cannot fix it. It should follow
`docs/rules/ui_design_rules.md` section 6: custom-painted surfaces must read colours
from a C++ token source keyed by the active theme and aligned with section 5.

Original rework requirement:

- Replace those raw chip colours with theme-aware token values.
- Reuse `ThemeManager::tokenValue(name, isDark)` if appropriate, or add a small
  C++ token helper that delegates to `ThemeManager`.
- Make the chip repaint on theme change if the widget stays alive during theme
  toggles.

## Remaining Findings To Fix

### 1. `DevicesMonitorWidget` stays dark inside `MitsubishiMcDeviceWidget`

`src/form/plc/mitsubishi_mc_device_widget.cpp` correctly registers
`mitsubishi_mc_device_widget_dark.qss` and
`mitsubishi_mc_device_widget_light.qss`, but it embeds two
`DevicesMonitorWidget` instances for M and D device monitoring. Those children
still own dark inline styling through `src/widgets/plc_widget/`:

- `DevicesMonitorWidget` builds styles in C++ with `setStyleSheet()`.
- `DevicesMonitorWidget` and `DeviceRowDelegate` still depend on
  `form/pattern/pattern_theme.h`, whose colours are not theme-token aware.
- The delegate paints table cells directly, so parent QSS cannot fully restyle
  the monitor rows.

Rework requirement:

- Make `DevicesMonitorWidget` and `DeviceRowDelegate` theme-aware.
- Prefer QSS/token based styling where possible. For custom-painted delegate
  surfaces, read colours from `ThemeManager::tokenValue(name, isDark)` or a
  small helper that delegates to `ThemeManager`.
- Subscribe to theme changes or otherwise re-apply styles and repaint while the
  widget is already open.
- Verify both embedded monitor instances in `MitsubishiMcDeviceWidget` switch
  cleanly between light and dark themes.

### 2. `SystemLogForm` is not themed to the new design

`src/system_log_form.cpp` only chooses a few message colours when a new log
entry arrives. It does not register a dark/light QSS pair, does not restyle the
form frame/controls through the token system, and existing entries are not
actively refreshed on theme changes.

Rework requirement:

- Add `system_log_form_dark.qss` and `system_log_form_light.qss` under
  `resrc/styles/`, register them in the Qt resource file, and wire
  `SystemLogForm` to the same theme reload pattern used by other forms.
- Replace hard-coded log severity colours with theme tokens or an approved
  token-backed C++ helper.
- Ensure the form background, controls, log area, and log text remain readable
  in both themes.
- Decide and document whether existing log entries should be recoloured on
  theme switch. Default expectation: visible entries should remain readable
  after toggling the theme.

## Acceptance Checklist

The rework is complete only when all items below are true:

- `DevicesMonitorWidget` follows the active app theme when embedded in
  `MitsubishiMcDeviceWidget`; the M and D monitor widgets no longer stay dark
  in light mode.
- `DeviceRowDelegate` custom painting uses theme-token colours rather than
  dark-only pattern constants.
- `SystemLogForm` has explicit dark and light styling that matches the design
  token direction.
- Log severity colours are token-backed or explicitly approved as raw
  exceptions.
- Regression check: `ThemeManager::tokenTable()` contains every token
  referenced by QSS.
- Regression check: no actual QSS selector contains a raw colour from
  `qss_sweep_map.md`, except approved raw exceptions.
- Regression check: `ProjectTreeWidget` does not reintroduce raw device
  identity colours in painter data.
- `docs/backlog/later_todo_list.md` and `docs/backlog/technical_debt_and_next_steps.md` do not
  claim full closeout until the above issues are fixed.
- Run the dark/light visual checklist from `docs/rules/ui_design_rules.md`.

## Suggested Verification

Token placeholder check:

```powershell
$refs = rg -o "@\{[A-Za-z0-9_.-]+\}" resrc/styles -g "*.qss" |
  ForEach-Object { if ($_ -match '@\{([^}]+)\}') { $matches[1] } } |
  Sort-Object -Unique
$defs = Get-Content src\utils\theme_manager.cpp |
  ForEach-Object { if ($_ -match '"([a-z0-9.\-]+)"\s*,\s*\{') { $matches[1] } } |
  Sort-Object -Unique
Compare-Object $defs $refs | Where-Object SideIndicator -eq '=>'
```

Sweep residual check:

```powershell
rg -n "#5a3800|#f0c0c0|#e0d0a0|#2b8ce8|#22d17a|#f5a623|#6b7ea0" `
  resrc/styles src/widgets/project_tree_widget.cpp -g "*.qss" -g "*.cpp"
```

Runtime contract smoke:

```powershell
$env:QT_QPA_PLATFORM='minimal'
$testBuild = Join-Path $env:NCR_PICKING_ROOT 'tests\architecture_contract_test\build\msvc_debug\debug'
Set-Location $testBuild
.\architecture_contract_test.exe -silent
```

Expected result: exit code `0`.
