#include "SmokeTests.h"

#include <QtTest/QtTest>

namespace RobotKinematics {
int libraryAnchor();
}

void SmokeTests::qtTestHarnessRuns()
{
    QCOMPARE(RobotKinematics::libraryAnchor(), 0);
}

int runSmokeTests(int argc, char** argv)
{
    SmokeTests tests;
    return QTest::qExec(&tests, argc, argv);
}
