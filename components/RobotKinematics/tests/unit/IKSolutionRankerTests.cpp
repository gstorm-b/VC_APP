#include "IKSolutionRankerTests.h"

#include <RobotKinematics/Solvers/IKSolutionRanker.h>

#include <QtTest/QtTest>

using namespace RobotKinematics;

void IKSolutionRankerTests::detectsPostureMatchAndMismatch()
{
    ArmPosture candidate;
    candidate.shoulder = -1;
    candidate.elbow = 1;

    ArmPosture requested;
    requested.shoulder = -1;
    QVERIFY(IKSolutionRanker::postureMatches(candidate, requested));

    requested.elbow = -1;
    QVERIFY(!IKSolutionRanker::postureMatches(candidate, requested));
}

void IKSolutionRankerTests::addsPostureMismatchCostWhenPreferenceIsSoft()
{
    SerialRobotConfig config;
    config.posture.resolver = "serial_6dof_shoulder_elbow_wrist";

    IKRequest request;
    request.posture = ArmPosture{};
    request.posture->shoulder = 1;

    IKSolution solution = IKSolutionRanker::score(
        config,
        request,
        JointVector::fromRadians({-0.1, 0.0, 0.2, 0.0, 0.3, 0.0}),
        0.0,
        0.0,
        0.05);

    QVERIFY(solution.score.postureMismatchCost > 0.0);
    QVERIFY(solution.score.totalCost >= solution.score.postureMismatchCost);
}

int runIKSolutionRankerTests(int argc, char** argv)
{
    IKSolutionRankerTests tests;
    return QTest::qExec(&tests, argc, argv);
}
