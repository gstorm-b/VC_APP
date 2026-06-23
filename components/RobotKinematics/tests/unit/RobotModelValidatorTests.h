#pragma once

#include <QObject>

class RobotModelValidatorTests : public QObject
{
    Q_OBJECT

private slots:
    void acceptsValidSerialRobotConfig();
    void reportsMissingBaseAndFlange();
    void reportsInvalidJointAxis();
    void reportsInvalidLimitsAndHome();
    void reportsMalformedSerialChain();
    void reportsMissingDefaultTool();
    void acceptsFixedJointBeyondConfiguredDof();
};
