#pragma once

#include <QObject>

class CollisionPrimitiveDistanceTests : public QObject
{
    Q_OBJECT

private slots:
    void sphereSphereDistancesCoverSeparatedTouchingAndOverlap();
    void sphereCapsuleDistanceReportsGapAndCollision();
    void capsuleCapsuleDistanceAndSafetyMarginAreApplied();
};
