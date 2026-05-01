#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include <optional>

#include <QObject>
#include <QPalette>
#include <QString>
#include <QVector>

// Describes a named visual style that can be registered with ThemeManager.
// Built-in styles "light" and "dark" are always pre-registered.
// Additional styles can be registered at runtime via ThemeManager::registerStyle().
struct ThemeStyle {
    QString id;                          // unique key, e.g. "dark", "high_contrast"
    QString displayName;                 // shown in the Theme menu
    bool    isDark{false};               // controls themedIcon() icon variant selection
    std::optional<QPalette> palette;     // nullopt = keep current palette unchanged
    QString qssPath;                     // Qt resource path to .qss file, empty = no stylesheet
};

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager* instance();

    // Register or replace a style. Emits styleRegistered if it is a new entry.
    // If the style id matches the active style, the new definition is applied immediately.
    void registerStyle(const ThemeStyle& style);

    QVector<ThemeStyle> styles() const { return m_styles; }

    // Switch to a registered style by id. No-op if id is unknown.
    void applyStyle(const QString& id);

    QString    currentStyleId() const { return m_currentId; }
    ThemeStyle currentStyle()   const;
    bool       isDark()         const;

    // Returns the appropriate icon resource path for the current theme.
    // Icons named "foo.svg" carry WHITE paths (for dark bg).
    // Icons named "foo_dark.svg" carry BLACK paths (for light bg).
    QString themedIcon(const QString& basePath) const;

signals:
    void themeChanged(QString styleId, bool isDark);
    void styleRegistered(ThemeStyle style);

private:
    explicit ThemeManager(QObject* parent = nullptr);
    void apply(const ThemeStyle& style);

    static QPalette buildLightPalette();
    static QPalette buildDarkPalette();

    QVector<ThemeStyle> m_styles;
    QString             m_currentId;
    static ThemeManager* s_instance;
};

#endif // THEME_MANAGER_H
