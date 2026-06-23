#include "IKApiTests.h"

#include <RobotKinematics/Kinematics/InverseKinematics.h>
#include <RobotKinematics/Solvers/IKSolver.h>

#include <QtTest/QtTest>

using namespace RobotKinematics;

namespace {
class RejectingSolver : public IKSolver
{
public:
    const char* name() const override { return "rejecting"; }

    IKResult solve(const SerialRobotConfig&, const IKSolveContext&) const override
    {
        return IKResult{IKStatus::UnsupportedSolver, {}, "unsupported"};
    }

    IKResult solveAll(const SerialRobotConfig& config, const IKSolveContext& context) const override
    {
        return solve(config, context);
    }
};
}

void IKApiTests::resultOkRequiresSolution()
{
    IKResult emptyOk;
    emptyOk.status = IKStatus::Ok;
    QVERIFY(!emptyOk.ok());

    IKSolution solution;
    solution.joints = JointVector::fromRadians({0.0});
    IKResult result;
    result.status = IKStatus::Ok;
    result.solutions.push_back(solution);

    QVERIFY(result.ok());
    QCOMPARE(result.best().joints.size(), 1);
}

void IKApiTests::requestDefaultsUseBaseFrameAndDefaultTool()
{
    IKRequest request;

    QVERIFY(request.referenceFrame.empty());
    QVERIFY(request.tool.empty());
    QVERIFY(request.options.returnClosestToSeed);
    QVERIFY(!request.options.requirePosture);
    QCOMPARE(request.options.maxSolutions, 16);
    QCOMPARE(request.options.maxPositionError_m, 1e-6);
    QCOMPARE(request.options.maxOrientationError_rad, 1.7453292519943296e-5);
}

void IKApiTests::solverInterfaceCanReturnStructuredFailure()
{
    RejectingSolver solver;
    SerialRobotConfig config;
    IKSolveContext context;

    const IKResult result = solver.solve(config, context);

    QCOMPARE(std::string(solver.name()), std::string("rejecting"));
    QVERIFY(!result.ok());
    QCOMPARE(result.status, IKStatus::UnsupportedSolver);
    QCOMPARE(result.message, std::string("unsupported"));
}

int runIKApiTests(int argc, char** argv)
{
    IKApiTests tests;
    return QTest::qExec(&tests, argc, argv);
}
