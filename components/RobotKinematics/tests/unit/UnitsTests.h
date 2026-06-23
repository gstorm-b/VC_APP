#pragma once

#include <QObject>

class UnitsTests : public QObject
{
    Q_OBJECT

private slots:
    void convertsMillimetersToMeters();
    void convertsMetersToMillimeters();
    void convertsDegreesToRadians();
    void convertsRadiansToDegrees();
};
