#include "theme_manager.h"
#include "app_settings/app_settings.h"

#include <QApplication>
#include <QFile>
#include <QStyle>
#include <QStyleFactory>

ThemeManager* ThemeManager::s_instance = nullptr;

ThemeManager* ThemeManager::instance() {
    if (!s_instance)
        s_instance = new ThemeManager(qApp);
    return s_instance;
}

ThemeManager::ThemeManager(QObject* parent) : QObject(parent) {
    qApp->setStyle(QStyleFactory::create("Fusion"));

    ThemeStyle light;
    light.id          = "light";
    light.displayName = tr("Light");
    light.isDark      = false;
    light.palette     = buildLightPalette();
    light.qssPath     = ":/styles/light.qss";
    registerStyle(light);

    ThemeStyle dark;
    dark.id          = "dark";
    dark.displayName = tr("Dark");
    dark.isDark      = true;
    dark.palette     = buildDarkPalette();
    dark.qssPath     = ":/styles/dark.qss";
    registerStyle(dark);

    applyStyle(AppSettings::instance()->theme());
}

void ThemeManager::registerStyle(const ThemeStyle& style) {
    for (auto& s : m_styles) {
        if (s.id == style.id) {
            s = style;
            if (m_currentId == style.id)
                apply(style);
            return;
        }
    }
    m_styles.append(style);
    emit styleRegistered(style);
}

void ThemeManager::applyStyle(const QString& id) {
    if (m_currentId == id) return;
    for (const auto& s : m_styles) {
        if (s.id == id) {
            m_currentId = id;
            apply(s);
            AppSettings::instance()->setTheme(id);
            emit themeChanged(id, s.isDark);
            return;
        }
    }
}

ThemeStyle ThemeManager::currentStyle() const {
    for (const auto& s : m_styles)
        if (s.id == m_currentId) return s;
    return {};
}

bool ThemeManager::isDark() const {
    return currentStyle().isDark;
}

QString ThemeManager::themedIcon(const QString& basePath) const {
    if (isDark()) return basePath;
    const int dot = basePath.lastIndexOf('.');
    if (dot < 0) return basePath;
    const QString darkPath = basePath.left(dot) + "_dark" + basePath.mid(dot);
    return QFile::exists(darkPath) ? darkPath : basePath;
}

void ThemeManager::apply(const ThemeStyle& style) {
    if (style.palette.has_value())
        qApp->setPalette(*style.palette);

    QString qss;
    if (!style.qssPath.isEmpty()) {
        QFile f(style.qssPath);
        if (f.open(QFile::ReadOnly | QFile::Text))
            qss = QString::fromUtf8(f.readAll());
    }
    qApp->setStyleSheet(qss);
}

QPalette ThemeManager::buildLightPalette() {
    QPalette p;
    // §5 design tokens — light theme
    const QColor bgWindow   {0xf2, 0xf3, 0xf5};  // bg.window
    const QColor bgSurface  {0xff, 0xff, 0xff};  // bg.surface
    const QColor bgElevated {0xe8, 0xea, 0xee};  // bg.elevated
    const QColor textPrimary{0x1a, 0x1c, 0x20};  // text.primary
    const QColor accent     {0xc0, 0x64, 0x00};  // accent.primary
    const QColor border     {0xc8, 0xcc, 0xd4};  // border.default
    const QColor disabled   {0xaa, 0xb0, 0xbc};  // text.disabled

    p.setColor(QPalette::Window,          bgWindow);
    p.setColor(QPalette::WindowText,      textPrimary);
    p.setColor(QPalette::Base,            bgSurface);
    p.setColor(QPalette::AlternateBase,   bgElevated);
    p.setColor(QPalette::Text,            textPrimary);
    p.setColor(QPalette::BrightText,      Qt::black);
    p.setColor(QPalette::Button,          bgElevated);
    p.setColor(QPalette::ButtonText,      textPrimary);
    p.setColor(QPalette::Highlight,       accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Light,           bgSurface);   // ADS focus_highlighting.css: palette(light) → bg.surface
    p.setColor(QPalette::Midlight,        bgElevated);
    p.setColor(QPalette::Dark,            border);      // ADS focus_highlighting.css: palette(dark) → border.default
    p.setColor(QPalette::Mid,             border);
    p.setColor(QPalette::Shadow,          QColor{0x80, 0x80, 0x80});
    p.setColor(QPalette::Link,            QColor{0x00, 0x66, 0xcc});
    p.setColor(QPalette::LinkVisited,     QColor{0x88, 0x00, 0xcc});
    p.setColor(QPalette::ToolTipBase,     bgSurface);
    p.setColor(QPalette::ToolTipText,     textPrimary);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text,       disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight,  border);

    return p;
}

QPalette ThemeManager::buildDarkPalette() {
    QPalette p;
    // §5 design tokens — dark theme
    const QColor bgWindow   {0x1a, 0x1c, 0x20};  // bg.window
    const QColor bgSurface  {0x22, 0x25, 0x2a};  // bg.surface
    const QColor bgElevated {0x2c, 0x30, 0x38};  // bg.elevated
    const QColor textPrimary{0xe0, 0xe4, 0xea};  // text.primary
    const QColor accent     {0xe8, 0x7c, 0x00};  // accent.primary
    const QColor border     {0x3a, 0x3f, 0x48};  // border.default
    const QColor disabled   {0x4a, 0x52, 0x60};  // text.disabled

    p.setColor(QPalette::Window,          bgWindow);
    p.setColor(QPalette::WindowText,      textPrimary);
    p.setColor(QPalette::Base,            bgWindow);
    p.setColor(QPalette::AlternateBase,   bgElevated);
    p.setColor(QPalette::Text,            textPrimary);
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Button,          bgElevated);
    p.setColor(QPalette::ButtonText,      textPrimary);
    p.setColor(QPalette::Highlight,       accent);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Light,           bgSurface);   // ADS focus_highlighting.css: palette(light) → bg.surface
    p.setColor(QPalette::Midlight,        bgElevated);
    p.setColor(QPalette::Dark,            border);      // ADS focus_highlighting.css: palette(dark) → border.default
    p.setColor(QPalette::Mid,             border);
    p.setColor(QPalette::Shadow,          QColor{0x11, 0x13, 0x16});
    p.setColor(QPalette::Link,            QColor{0x4f, 0xc1, 0xff});
    p.setColor(QPalette::LinkVisited,     QColor{0xb5, 0x89, 0xff});
    p.setColor(QPalette::ToolTipBase,     bgSurface);
    p.setColor(QPalette::ToolTipText,     textPrimary);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text,       disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight,  QColor{0x55, 0x55, 0x55});

    return p;
}
