#pragma once

#include <QObject>

class NumericalIKSolverTests : public QObject
{
    Q_OBJECT

private slots:
    void convergesToKnownSixDofFixtureFromNearbySeed();
    void rejectsSeedDimensionMismatch();
    void reportsFailureWhenIterationBudgetIsTooSmall();
};
