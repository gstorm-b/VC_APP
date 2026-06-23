#pragma once

#include <QObject>

class StlPrimitiveAuthoringHelperTests : public QObject
{
    Q_OBJECT

private slots:
    void proposesConservativePrimitivesFromAsciiStl();
    void proposesConservativePrimitivesFromBinaryStl();
    void rejectsInvalidStlPayload();
};

int runStlPrimitiveAuthoringHelperTests(int argc, char** argv);
