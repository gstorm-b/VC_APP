#include "PostureResolverTests.h"

#include <RobotKinematics/Posture/PostureResolver.h>

#include <QtTest/QtTest>

using namespace RobotKinematics;

namespace {
PostureMetadata metadata()
{
    PostureMetadata posture;
    posture.resolver = "serial_6dof_shoulder_elbow_wrist";
    posture.labels["shoulder"] = PostureLabelAxis{"lefty", "righty"};
    posture.labels["elbow"] = PostureLabelAxis{"below", "above"};
    posture.labels["wrist"] = PostureLabelAxis{"non-flip", "flip"};
    return posture;
}
}

void PostureResolverTests::mapsConfiguredLabelsToGenericBranches()
{
    const Serial6DofSignPostureResolver resolver;
    const Result<ArmPosture> posture = resolver.fromLabels(metadata(), {
        {"shoulder", "lefty"},
        {"elbow", "above"},
        {"wrist", "flip"},
    });

    QVERIFY(posture.ok());
    QCOMPARE(*posture.value.shoulder, -1);
    QCOMPARE(*posture.value.elbow, 1);
    QCOMPARE(*posture.value.wrist, 1);
}

void PostureResolverTests::classifiesSerialSixDofSignBranches()
{
    SerialRobotConfig config;
    const Serial6DofSignPostureResolver resolver;

    const Result<ArmPosture> posture =
        resolver.classify(config, JointVector::fromRadians({-0.1, 0.0, 0.2, 0.0, -0.3, 0.0}));

    QVERIFY(posture.ok());
    QCOMPARE(*posture.value.shoulder, -1);
    QCOMPARE(*posture.value.elbow, 1);
    QCOMPARE(*posture.value.wrist, -1);
}

int runPostureResolverTests(int argc, char** argv)
{
    PostureResolverTests tests;
    return QTest::qExec(&tests, argc, argv);
}
