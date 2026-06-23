#pragma once

#include <QObject>

class Virtual6DofTestArmTests : public QObject
{
    Q_OBJECT

private slots:
    void fallbackPresetIsValidAndHasRequiredMetadata();
    void jsonPresetMatchesCppFallbackForSolverFacingFields();
    void presetRunsFkAndSeededIkRoundTrip();
};
