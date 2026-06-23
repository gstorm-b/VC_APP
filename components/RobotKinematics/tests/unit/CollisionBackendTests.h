#pragma once

#include <QObject>

class CollisionBackendTests : public QObject
{
    Q_OBJECT

private slots:
    void primitiveBackendReportsBuiltInCapabilities();
    void meshBackendReportsUnavailableWithoutCompiledAdapter();
    void primitiveChecksFlowThroughBackendFacade();
    void meshChecksReturnStructuredUnsupportedStatus();
    void meshChecksDetectSyntheticOverlapAndSeparationWhenCompiled();
    void meshChecksApplySafetyMarginWhenCompiled();
};
