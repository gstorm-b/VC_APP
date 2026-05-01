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
    const QColor bg        {0xf0, 0xf0, 0xf0};
    const QColor base      {0xff, 0xff, 0xff};
    const QColor text      {0x1e, 0x1e, 0x1e};
    const QColor btn       {0xe1, 0xe1, 0xe1};
    const QColor highlight {0x00, 0x78, 0xd4};
    const QColor light     {0xff, 0xff, 0xff};
    const QColor mid       {0xd0, 0xd0, 0xd0};
    const QColor dark      {0xa0, 0xa0, 0xa0};
    const QColor disabled  {0xa0, 0xa0, 0xa0};

    p.setColor(QPalette::Window,          bg);
    p.setColor(QPalette::WindowText,      text);
    p.setColor(QPalette::Base,            base);
    p.setColor(QPalette::AlternateBase,   QColor{0xf5, 0xf5, 0xf5});
    p.setColor(QPalette::Text,            text);
    p.setColor(QPalette::BrightText,      Qt::black);
    p.setColor(QPalette::Button,          btn);
    p.setColor(QPalette::ButtonText,      text);
    p.setColor(QPalette::Highlight,       highlight);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Light,           light);
    p.setColor(QPalette::Midlight,        QColor{0xf5, 0xf5, 0xf5});
    p.setColor(QPalette::Dark,            dark);
    p.setColor(QPalette::Mid,             mid);
    p.setColor(QPalette::Shadow,          QColor{0x80, 0x80, 0x80});
    p.setColor(QPalette::Link,            QColor{0x00, 0x66, 0xcc});
    p.setColor(QPalette::LinkVisited,     QColor{0x88, 0x00, 0xcc});
    p.setColor(QPalette::ToolTipBase,     base);
    p.setColor(QPalette::ToolTipText,     text);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text,       disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight,  QColor{0xc8, 0xc8, 0xc8});

    return p;
}

QPalette ThemeManager::buildDarkPalette() {
    QPalette p;
    const QColor bg        {0x2d, 0x2d, 0x2d};
    const QColor base      {0x25, 0x25, 0x26};
    const QColor text      {0xcc, 0xcc, 0xcc};
    const QColor btn       {0x3c, 0x3c, 0x3c};
    const QColor highlight {0x09, 0x47, 0x71};
    const QColor dark      {0x1e, 0x1e, 0x1e};
    const QColor mid       {0x45, 0x45, 0x45};
    const QColor disabled  {0x6e, 0x6e, 0x6e};

    p.setColor(QPalette::Window,          bg);
    p.setColor(QPalette::WindowText,      text);
    p.setColor(QPalette::Base,            base);
    p.setColor(QPalette::AlternateBase,   btn);
    p.setColor(QPalette::Text,            text);
    p.setColor(QPalette::BrightText,      Qt::white);
    p.setColor(QPalette::Button,          btn);
    p.setColor(QPalette::ButtonText,      text);
    p.setColor(QPalette::Highlight,       highlight);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Light,           btn);
    p.setColor(QPalette::Midlight,        QColor{0x38, 0x38, 0x38});
    p.setColor(QPalette::Dark,            dark);
    p.setColor(QPalette::Mid,             mid);
    p.setColor(QPalette::Shadow,          QColor{0x10, 0x10, 0x10});
    p.setColor(QPalette::Link,            QColor{0x4f, 0xc1, 0xff});
    p.setColor(QPalette::LinkVisited,     QColor{0xb5, 0x89, 0xff});
    p.setColor(QPalette::ToolTipBase,     base);
    p.setColor(QPalette::ToolTipText,     text);

    p.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Text,       disabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    p.setColor(QPalette::Disabled, QPalette::Highlight,  QColor{0x55, 0x55, 0x55});

    return p;
}
