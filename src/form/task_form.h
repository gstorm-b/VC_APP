#ifndef TASK_FORM_H
#define TASK_FORM_H

#include <QWidget>

namespace Ui {
class TaskForm;
}

class TaskForm : public QWidget
{
    Q_OBJECT

public:
    explicit TaskForm(QWidget *parent = nullptr);
    ~TaskForm();

private:
    Ui::TaskForm *ui;
};

#endif // TASK_FORM_H
