#ifndef PATTERN_THEME_H
#define PATTERN_THEME_H

#include <QString>
#include <QColor>

// ─────────────────────────────────────────────────────────────────────────────
//  Pattern UI theme tokens
//
//  Mirrors the design handoff (`ui_scratch/design_handoff_full_project`).
//  Every color used in the pattern manager / wizards lives here so that the
//  design and implementation never drift.
//
//  Values are taken verbatim from `README.md` "Design Tokens" table.
// ─────────────────────────────────────────────────────────────────────────────

namespace ptn {

// ── Surfaces ─────────────────────────────────────────────────────────────────
constexpr const char* BG     = "#1e1e1e";   // Page background, panels
constexpr const char* SURF   = "#252526";   // Cards, headers
constexpr const char* SURF2  = "#2d2d2d";   // Elevated modals
constexpr const char* HD     = "#1e1e1e";   // Header strip background

// ── Borders ──────────────────────────────────────────────────────────────────
constexpr const char* BD     = "#3c3c3c";   // Default borders
constexpr const char* BD2    = "#454545";   // Input borders, hover borders

// ── Text ─────────────────────────────────────────────────────────────────────
constexpr const char* TXT    = "#cccccc";   // Primary text
constexpr const char* TXT2   = "#9a9a9a";   // Secondary text
constexpr const char* TXT3   = "#7a7a7a";   // Muted / hint text
constexpr const char* TXT4   = "#5a5a5a";   // Disabled text
constexpr const char* TXT5   = "#454545";   // Placeholder / label text

// ── Accents ──────────────────────────────────────────────────────────────────
constexpr const char* ACC    = "#2b8ce8";   // Primary accent (blue)
constexpr const char* OK     = "#22d17a";   // Success / connected green
constexpr const char* WARN   = "#f5a623";   // Warning orange
constexpr const char* ERR    = "#e84040";   // Error red
constexpr const char* OUTPUT = "#9cdcfe";   // PLC output ID (light blue)

// ── Edit Pattern Wizard "lock" color ────────────────────────────────────────
constexpr const char* LOCK   = "#6a5acd";   // Purple for locked/identity step

// ── Fonts ────────────────────────────────────────────────────────────────────
constexpr const char* FONT_SANS = "Space Grotesk, Segoe UI, sans-serif";
constexpr const char* FONT_MONO = "JetBrains Mono, Consolas, monospace";

// ─────────────────────────────────────────────────────────────────────────────
//  Reusable QSS fragments
// ─────────────────────────────────────────────────────────────────────────────

// Apply to top-level pattern widget / wizard to set base background + text.
inline QString baseStyleSheet() {
    return QString(
        "QWidget { background-color: %1; color: %2; font-family: %3; }"
        "QLabel  { background: transparent; }"
        "QFrame  { background: transparent; }"
    ).arg(BG, TXT, FONT_SANS);
}

// Section header label (uppercase, bold, muted).
inline QString sectionHeaderStyle() {
    return QString(
        "background-color: %1; color: %2; "
        "font: 700 9pt \"%3\"; letter-spacing: 1.2px; "
        "padding: 4px 10px; border-bottom: 1px solid %4;"
    ).arg(HD, TXT3, "Segoe UI", BD);
}

// Toolbar frame style.
inline QString toolbarStyle() {
    return QString(
        "QFrame#toolbar { background-color: %1; border-bottom: 1px solid %2; }"
        "QFrame#toolbar QToolButton { color: %3; background: transparent; "
        "  border: 1px solid transparent; padding: 4px 12px; border-radius: 4px; "
        "  font: 600 10pt \"Segoe UI\"; }"
        "QFrame#toolbar QToolButton:hover { background-color: %4; border: 1px solid %5; color: %6; }"
        "QFrame#toolbar QToolButton:pressed { background-color: %1; }"
        "QFrame#toolbar QToolButton:disabled { color: %7; }"
        "QFrame#toolbar QLabel  { color: %3; font: 10pt \"Segoe UI\"; }"
    ).arg(HD, BD, TXT2, SURF2, BD2, TXT, TXT4);
}

// Input field style (line edits, spinboxes).
inline QString inputStyle() {
    return QString(
        "QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox {"
        "  background-color: %1; color: %2; "
        "  border: 1px solid %3; border-radius: 4px; "
        "  padding: 4px 8px; "
        "  font: 11pt \"%4\"; }"
        "QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus {"
        "  border: 1px solid %5; }"
        "QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled {"
        "  color: %6; }"
    ).arg(BG, TXT, BD2, FONT_MONO, ACC, TXT4);
}

// Primary button (bold blue).
inline QString primaryButtonStyle() {
    return QString(
        "QPushButton {"
        "  background-color: %1; color: white; border: none; border-radius: 4px;"
        "  padding: 7px 18px; font: 700 10pt \"Segoe UI\"; }"
        "QPushButton:hover { background-color: #3a9ef5; }"
        "QPushButton:disabled { background-color: %2; color: %3; }"
    ).arg(ACC, SURF2, TXT4);
}

// Ghost button (secondary).
inline QString ghostButtonStyle() {
    return QString(
        "QPushButton {"
        "  background: transparent; color: %1; "
        "  border: 1px solid %2; border-radius: 4px;"
        "  padding: 5px 12px; font: 600 10pt \"Segoe UI\"; }"
        "QPushButton:hover { background-color: %3; color: %4; border-color: %5; }"
        "QPushButton:disabled { color: %6; border-color: %2; }"
    ).arg(TXT2, BD, SURF2, TXT, BD2, TXT4);
}

// Active step pill (in wizard step rail).
inline QString stepBubbleStyle(bool done, bool current) {
    const char *bg     = done ? OK : current ? ACC : SURF;
    const char *fg     = (done || current) ? "white" : TXT3;
    const char *border = done ? OK : current ? ACC : BD2;
    return QString(
        "QLabel { background-color: %1; color: %2; "
        "  border: 1px solid %3; border-radius: 11px; "
        "  font: 700 9pt \"%4\"; min-width: 22px; max-width: 22px; "
        "  min-height: 22px; max-height: 22px; }"
    ).arg(bg, fg, border, FONT_MONO);
}

} // namespace ptn

#endif // PATTERN_THEME_H
