#ifndef SYSTEM_LOG_FORM_H
#define SYSTEM_LOG_FORM_H

#include <QWidget>
#include "logger/app_logger.h"

namespace Ui {
class SystemLogForm;
}

class SystemLogForm : public QWidget {
    Q_OBJECT
public:
    explicit SystemLogForm(QWidget *parent = nullptr);
    ~SystemLogForm();

private slots:
    void onNewLogReceived(const LogMessage &msg);

private:
    Ui::SystemLogForm *ui;
};

#endif // SYSTEM_LOG_FORM_H
