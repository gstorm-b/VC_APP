#include "task_form.h"
#include "ui_task_form.h"

TaskForm::TaskForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::TaskForm)
{
    ui->setupUi(this);
}

TaskForm::~TaskForm()
{
    delete ui;
}
