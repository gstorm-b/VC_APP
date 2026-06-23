#pragma once

#include <QObject>

class SmokeTests : public QObject
{
    Q_OBJECT

private slots:
    void qtTestHarnessRuns();
};
