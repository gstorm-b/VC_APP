#include "system_log_form.h"
#include "ui_system_log_form.h"
#include "utils/theme_manager.h"

SystemLogForm::SystemLogForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SystemLogForm) {
    ui->setupUi(this);


    QStringList viewModeList = {tr("User"), tr("Developer")};
    ui->cbx_view_mode->addItems(viewModeList);


    connect(&AppLogger::instance(), &AppLogger::newLogAdded,
            this, &SystemLogForm::onNewLogReceived);

}

SystemLogForm::~SystemLogForm() {
    delete ui;
}

void SystemLogForm::onNewLogReceived(const LogMessage &msg) {
    if (ui->cbx_view_mode->currentIndex() != 1 &&
        msg.category == LogCategory::Developer)
        return;

    const bool dark = ThemeManager::instance()->isDark();

    // §5 state tokens — chosen per-entry at append time so new entries
    // reflect the current theme without needing a themeChanged subscription.
    QString color;
    if (msg.level == LogLevel::Warning)
        color = dark ? QStringLiteral("#ffb020") : QStringLiteral("#9a6800");  // state.warning
    else if (msg.level == LogLevel::Error)
        color = dark ? QStringLiteral("#ff5252") : QStringLiteral("#c0282a");  // state.error
    else if (msg.level == LogLevel::Debug)
        color = dark ? QStringLiteral("#40c870") : QStringLiteral("#1a7a40");  // state.success
    else
        color = dark ? QStringLiteral("#e0e4ea") : QStringLiteral("#1a1c20");  // text.primary

    const QString timeStr = msg.timestamp.toString("hh:mm:ss");
    const QString catStr  = (msg.category == LogCategory::User)
                            ? QStringLiteral("[USER]") : QStringLiteral("[DEV]");
    const QString ctx     = msg.context.isEmpty()
                            ? QString()
                            : QStringLiteral(" <i>(") + msg.context + QStringLiteral(")</i>");

    ui->log_view->appendHtml(
        QStringLiteral("<span style='color: %1;'>[%2]%3 %4%5</span>")
            .arg(color, timeStr, catStr, msg.message, ctx));
}
