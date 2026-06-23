#pragma once

#include <QObject>

class JointLimitValidatorTests : public QObject
{
    Q_OBJECT

private slots:
    void jointVectorConvertsRadiansAndDegrees();
    void acceptsJointsInsideLimits();
    void rejectsJointBelowMinimum();
    void rejectsJointAboveMaximum();
    void rejectsWrongDimension();
};
