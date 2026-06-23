#pragma once

#include <QObject>

class RobotModelConfigTests : public QObject
{
    Q_OBJECT

private slots:
    void representsSerialSixDofRobotConfig();
    void supportsOptionalVelocityAndAccelerationLimits();
};
