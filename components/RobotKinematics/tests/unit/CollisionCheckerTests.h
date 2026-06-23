#pragma once

#include <QObject>

class CollisionCheckerTests : public QObject
{
    Q_OBJECT

private slots:
    void transformsGeometryIntoBaseAndKeepsDeterministicPairOrder();
    void skipsDisabledGeometryAndDisabledPairs();
    void invalidJointDimensionReturnsStructuredFailure();
    void stopsAfterFirstCollisionWhenReturnAllPairsIsFalse();
};
