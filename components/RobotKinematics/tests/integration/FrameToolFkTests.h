#pragma once

#include <QObject>

class FrameToolFkTests : public QObject
{
    Q_OBJECT

private slots:
    void toolPoseComposesFlangeAndTcp();
    void userFrameRelativePose();
    void jointLimitValidatorRejectsWrongDimension();
};
