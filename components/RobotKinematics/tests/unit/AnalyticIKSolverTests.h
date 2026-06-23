#pragma once

#include <QObject>

class AnalyticIKSolverTests : public QObject
{
    Q_OBJECT

private slots:
    void supportsSphericalWristModel();
    void rejectsNonSphericalWristModel();
    void unsupportedModelFallsBackToNumerical();
    void analyticSolveAllBranchesReproduceTargetPose();
    void analyticSolveRecoversSeededConfiguration();
};
