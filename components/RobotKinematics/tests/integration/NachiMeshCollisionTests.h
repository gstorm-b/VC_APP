#pragma once

#include <QObject>

class NachiMeshCollisionTests : public QObject
{
    Q_OBJECT

private slots:
    void profileLoadsAndValidatesAgainstNachiPreset();
    void everyStlMeshLoadsWithMillimeterScale();
    void meshToLinkTransformsReproduceVisualizerHomePlacement();
    void meshBackendDetectsHomePoseHasNoCollisionWhenCompiled();
    void meshBackendDetectsKnownSelfCollisionPoseWhenCompiled();
};

int runNachiMeshCollisionTests(int argc, char** argv);
