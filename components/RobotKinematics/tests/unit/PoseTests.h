#pragma once

#include <QObject>

class PoseTests : public QObject
{
    Q_OBJECT

private slots:
    void identityHasNoTranslationOrRotation();
    void fromIsometryPreservesTransform();
    void fromXYZRPY_m_radUsesFixedAxisRpyConvention();
    void fromXYZRPY_mm_degConvertsExplicitUnits();
    void composesTransforms();
};
