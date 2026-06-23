#pragma once

#include <QObject>

class IKIntegrationTests : public QObject
{
    Q_OBJECT

private slots:
    void solveUsesDefaultToolAndReturnsBestSolution();
    void solveAllReturnsFoundSolutions();
    void missingToolAndFrameReturnStructuredStatus();
    void solveResolvesBaseFixedUserFrameAndTool();
    void requirePostureRejectsMismatchedSolution();
};
