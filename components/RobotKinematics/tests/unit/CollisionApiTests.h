#pragma once

#include <QObject>

class CollisionApiTests : public QObject
{
    Q_OBJECT

private slots:
    void requestDefaultsReturnAllPairsWithZeroSafetyMargin();
    void resultOkDependsOnStatusNotCollisionFlag();
    void profileCanStoreSphereAndCapsuleGeometry();
};
