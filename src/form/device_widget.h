#ifndef DEVICE_WIDGET_H
#define DEVICE_WIDGET_H

#include <QWidget>
#include <QObject>

class IDeviceWidget : public QWidget {
    Q_OBJECT

public:
    explicit IDeviceWidget(QWidget *parent = nullptr) {}

    virtual void loadConfigToDevice() = 0;
    virtual void loadConfigToWidget() = 0;
};

#endif // DEVICE_WIDGET_H
