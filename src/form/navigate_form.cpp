#include "navigate_form.h"
#include "ui_navigate_form.h"

NavigateForm::NavigateForm(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::NavigateForm)
{
    ui->setupUi(this);
}

NavigateForm::~NavigateForm()
{
    delete ui;
}
