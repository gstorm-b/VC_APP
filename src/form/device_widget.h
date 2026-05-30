#ifndef DEVICE_WIDGET_H
#define DEVICE_WIDGET_H

#include <QWidget>
#include <QObject>
#include "widgets/property_browser/property_browser_widget.h"

class IDeviceWidget : public QWidget {
    Q_OBJECT

public:
    explicit IDeviceWidget(QWidget *parent = nullptr) {}

    virtual PropertyBrowserWidget* getPropertyBrowser() const {
        return m_propBrowser;
    }

    virtual QString deviceId() = 0;
    virtual void loadConfigToDevice() = 0;
    virtual void loadConfigToWidget() = 0;

protected:
    // ── Initialisation ────────────────────────────────────────────────────
    void initPropertyBrowser() {
        m_propBrowser = new PropertyBrowserWidget(this);

        m_variantManager = m_propBrowser->variantManager();
        m_variantFactory  = m_propBrowser->variantFactory();
        m_variantEditor   = m_propBrowser->browser();

        // if (container) embedBrowserInWidget(container);
    }

    // void embedBrowserInWidget(QWidget *container) {
    //     auto *layout = new QHBoxLayout(container);
    //     layout->setContentsMargins(0, 0, 0, 0);
    //     layout->addWidget(m_propBrowser);
    // }

        // High-level wrapper (created by initPropertyBrowser)
    PropertyBrowserWidget          *m_propBrowser{nullptr};

    // Low-level pointers — kept for backward compatibility.
    // They point into m_propBrowser's internals; do not delete them.
    QtVariantPropertyManager *m_variantManager{nullptr};
    QtVariantEditorFactory   *m_variantFactory{nullptr};
    QtTreePropertyBrowser    *m_variantEditor {nullptr};
};

#endif // DEVICE_WIDGET_H
