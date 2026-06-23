#pragma once

#include <QObject>

class IKSolutionRankerTests : public QObject
{
    Q_OBJECT

private slots:
    void detectsPostureMatchAndMismatch();
    void addsPostureMismatchCostWhenPreferenceIsSoft();
};
