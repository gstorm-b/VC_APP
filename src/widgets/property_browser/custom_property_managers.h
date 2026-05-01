#pragma once

#include "widgets/qtpropertybrowser/qtpropertybrowser.h"
#include "widgets/qtpropertybrowser/qtpropertymanager.h"

#include <QVector>
#include <QStringList>

// ============================================================================
//  PositionPropertyManager
//
//  A compound property for robot / machine positions.
//  Three display modes:
//    XY     — X, Y                   (2 components, e.g. image-plane point)
//    XYZ    — X, Y, Z                (3 components, e.g. 3-D Cartesian)
//    XYZRPY — X, Y, Z, Roll, Pitch, Yaw (6-DOF robot pose)
//
//  Each component is a QtDoubleProperty created internally.
//  The sub-properties must have their editor factory registered in the
//  browser:
//      browser->setFactoryForManager(posMan->subDoubleManager(), &dblFactory);
//
//  Example:
//      auto *pos = posMan->addProperty("TCP Position");
//      posMan->setMode(pos, PositionPropertyManager::XYZRPY);
//      posMan->setRange(pos, -1000.0, 1000.0);
//      posMan->setDecimals(pos, 2);
//      posMan->setValue(pos, {100.0, 200.0, 300.0, 0.0, 0.0, 45.0});
//      browser->addProperty(pos);
// ============================================================================

class PositionPropertyManager : public QtAbstractPropertyManager {
    Q_OBJECT
public:
    enum Mode { XY = 0, XYZ = 1, XYZRPY = 2 };
    Q_ENUM(Mode)

    static int         modeComponentCount(Mode m);
    static QStringList modeLabels(Mode m);

    explicit PositionPropertyManager(QObject *parent = nullptr);
    ~PositionPropertyManager() override;

    QtDoublePropertyManager *subDoubleManager() const;

    // Value accessors
    QVector<double> value     (const QtProperty *property) const;
    Mode            mode      (const QtProperty *property) const;
    int             decimals  (const QtProperty *property) const;
    double          minimum   (const QtProperty *property) const;
    double          maximum   (const QtProperty *property) const;
    double          singleStep(const QtProperty *property) const;

public slots:
    void setValue    (QtProperty *property, const QVector<double> &val);
    void setMode     (QtProperty *property, Mode mode);
    void setDecimals (QtProperty *property, int prec);
    void setRange    (QtProperty *property, double minVal, double maxVal);
    void setSingleStep(QtProperty *property, double step);

signals:
    void valueChanged   (QtProperty *property, const QVector<double> &val);
    void modeChanged    (QtProperty *property, Mode mode);
    void decimalsChanged(QtProperty *property, int prec);

protected:
    QString valueText(const QtProperty *property) const override;
    void    initializeProperty  (QtProperty *property) override;
    void    uninitializeProperty(QtProperty *property) override;

private slots:
    void slotDoubleChanged    (QtProperty *sub, double value);
    void slotPropertyDestroyed(QtProperty *sub);

private:
    struct Data {
        QVector<double>    values;
        Mode               mode{XY};
        int                decimals{3};
        double             minimum{-1e6};
        double             maximum{+1e6};
        double             singleStep{0.001};
        QList<QtProperty*> subProps;
    };

    void createSubProperties(QtProperty *parent, Data &data);
    void destroySubProperties(QtProperty *parent, Data &data);

    QMap<const QtProperty *, Data>   m_values;
    QMap<QtProperty *, QtProperty *> m_subToParent;
    QtDoublePropertyManager         *m_doubleManager;

    Q_DISABLE_COPY(PositionPropertyManager)
};

// ============================================================================
//  SizePropertyManager
//
//  A compound property for physical or pixel sizes.
//  Two display modes:
//    WH  — Width, Height          (2-D, e.g. image dimensions, pick-box)
//    WHD — Width, Height, Depth   (3-D, e.g. bounding box)
//
//  Same factory-registration requirement as PositionPropertyManager.
// ============================================================================

class SizePropertyManager : public QtAbstractPropertyManager {
    Q_OBJECT
public:
    enum Mode { WH = 0, WHD = 1 };
    Q_ENUM(Mode)

    static int         modeComponentCount(Mode m);
    static QStringList modeLabels(Mode m);

    explicit SizePropertyManager(QObject *parent = nullptr);
    ~SizePropertyManager() override;

    QtDoublePropertyManager *subDoubleManager() const;

    QVector<double> value     (const QtProperty *property) const;
    Mode            mode      (const QtProperty *property) const;
    int             decimals  (const QtProperty *property) const;
    double          minimum   (const QtProperty *property) const;
    double          maximum   (const QtProperty *property) const;
    double          singleStep(const QtProperty *property) const;

public slots:
    void setValue    (QtProperty *property, const QVector<double> &val);
    void setMode     (QtProperty *property, Mode mode);
    void setDecimals (QtProperty *property, int prec);
    void setRange    (QtProperty *property, double minVal, double maxVal);
    void setSingleStep(QtProperty *property, double step);

signals:
    void valueChanged   (QtProperty *property, const QVector<double> &val);
    void modeChanged    (QtProperty *property, Mode mode);
    void decimalsChanged(QtProperty *property, int prec);

protected:
    QString valueText(const QtProperty *property) const override;
    void    initializeProperty  (QtProperty *property) override;
    void    uninitializeProperty(QtProperty *property) override;

private slots:
    void slotDoubleChanged    (QtProperty *sub, double value);
    void slotPropertyDestroyed(QtProperty *sub);

private:
    struct Data {
        QVector<double>    values;
        Mode               mode{WH};
        int                decimals{3};
        double             minimum{0.0};
        double             maximum{+1e6};
        double             singleStep{0.001};
        QList<QtProperty*> subProps;
    };

    void createSubProperties(QtProperty *parent, Data &data);
    void destroySubProperties(QtProperty *parent, Data &data);

    QMap<const QtProperty *, Data>   m_values;
    QMap<QtProperty *, QtProperty *> m_subToParent;
    QtDoublePropertyManager         *m_doubleManager;

    Q_DISABLE_COPY(SizePropertyManager)
};
