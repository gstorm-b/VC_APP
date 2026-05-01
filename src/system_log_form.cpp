#include "system_log_form.h"
#include "ui_system_log_form.h"

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
    bool isDevMode = (ui->cbx_view_mode->currentIndex() == 1);

    // log filter, ignore dev log in user view mode
    if (!isDevMode && msg.category == LogCategory::Developer) {
        return;
    }

    // log color level
    QString color = "#000000"; // infor black
    if (msg.level == LogLevel::Warning) color = "#ff8c00"; // warning orange
    else if (msg.level == LogLevel::Error) color = "#ff0000"; // error red
    else if (msg.level == LogLevel::Debug) color = "##00ff33"; // debug green

    QString timeStr = msg.timestamp.toString("hh:mm:ss");
    QString catStr = (msg.category == LogCategory::User) ? "[USER]" : "[DEV]";

    // HTML log format
    QString html = QString("<span style='color: %1;'>[%2]%3 %4 %5</span>")
                       .arg(color)
                       .arg(timeStr)
                       .arg(catStr)
                       .arg(msg.message)
                       .arg(msg.context.isEmpty() ? "" : "<i>(" + msg.context + ")</i>");

    ui->log_view->appendHtml(html);
}
