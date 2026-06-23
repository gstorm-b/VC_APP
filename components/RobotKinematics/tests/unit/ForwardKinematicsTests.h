#pragma once

#include <QObject>

class ForwardKinematicsTests : public QObject
{
    Q_OBJECT

private slots:
    void singleRevoluteJointWithOffset();
    void twoJointPlanarArmWithTool();
    void sixJointTowerComposition();
};
