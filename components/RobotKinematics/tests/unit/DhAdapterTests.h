#pragma once

#include <QObject>

class DhAdapterTests : public QObject
{
    Q_OBJECT

private slots:
    void standardDhPlanarTwoJointMatchesExpectedFk();
};
