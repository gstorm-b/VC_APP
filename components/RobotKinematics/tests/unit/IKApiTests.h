#pragma once

#include <QObject>

class IKApiTests : public QObject
{
    Q_OBJECT

private slots:
    void resultOkRequiresSolution();
    void requestDefaultsUseBaseFrameAndDefaultTool();
    void solverInterfaceCanReturnStructuredFailure();
};
