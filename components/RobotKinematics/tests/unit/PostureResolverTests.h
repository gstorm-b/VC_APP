#pragma once

#include <QObject>

class PostureResolverTests : public QObject
{
    Q_OBJECT

private slots:
    void mapsConfiguredLabelsToGenericBranches();
    void classifiesSerialSixDofSignBranches();
};
