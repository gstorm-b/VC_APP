#ifndef NAVIGATE_FORM_H
#define NAVIGATE_FORM_H

#include <QWidget>

namespace Ui {
class NavigateForm;
}

class NavigateForm : public QWidget
{
    Q_OBJECT

public:
    explicit NavigateForm(QWidget *parent = nullptr);
    ~NavigateForm();

private:
    Ui::NavigateForm *ui;
};

#endif // NAVIGATE_FORM_H
