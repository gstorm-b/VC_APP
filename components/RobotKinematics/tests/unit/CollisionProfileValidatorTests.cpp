#include "CollisionProfileValidatorTests.h"

#include <RobotKinematics/Collision/CollisionProfileValidator.h>
#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Model/RobotModelConfig.h>

#include <Eigen/Core>

#include <QtTest/QtTest>

#include <optional>

using namespace RobotKinematics;

namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;

Joint revoluteZ(const std::string& id, const std::string& parent, const std::string& child)
{
    Joint joint;
    joint.id = id;
    joint.type = JointType::Revolute;
    joint.parentLinkId = parent;
    joint.childLinkId = child;
    joint.axis = Eigen::Vector3d::UnitZ();
    joint.origin = Pose::identity();
    joint.limits = JointLimits{-kPi, kPi, std::nullopt, std::nullopt};
    joint.home = 0.0;
    return joint;
}

SerialRobotConfig threeLinkRobot()
{
    SerialRobotConfig config;
    config.identity = RobotIdentity{"RobotKinematics", "CollisionFixture", "Collision Fixture", "1.0.0"};
    config.topology = RobotTopologyType::Serial;
    config.dof = 2;
    config.links = {{"base_link"}, {"link_1"}, {"flange"}};
    config.joints = {
        revoluteZ("J1", "base_link", "link_1"),
        revoluteZ("J2", "link_1", "flange"),
    };
    config.frames.baseLinkId = "base_link";
    config.frames.flangeLinkId = "flange";
    config.tools = {Tool{"default", "Default Tool", Pose::identity()}};
    config.defaultToolId = "default";
    return config;
}

CollisionProfile validProfile()
{
    CollisionProfile profile;
    profile.id = "fixture_profile";
    profile.robotModel = "CollisionFixture";

    CollisionGeometry sphere;
    sphere.id = "base_body";
    sphere.linkId = "base_link";
    sphere.shape.type = CollisionShapeType::Sphere;
    sphere.shape.sphere.radius_m = 0.12;

    CollisionGeometry capsule;
    capsule.id = "link_1_body";
    capsule.linkId = "link_1";
    capsule.shape.type = CollisionShapeType::Capsule;
    capsule.shape.capsule.radius_m = 0.05;
    capsule.shape.capsule.length_m = 0.30;

    profile.geometries = {sphere, capsule};
    profile.disabledPairs = {
        DisabledCollisionPair{"base_body", "link_1_body", "adjacent_joint_contact"},
    };
    return profile;
}
}

void CollisionProfileValidatorTests::validProfilePasses()
{
    const CollisionProfileValidationResult result =
        CollisionProfileValidator::validate(threeLinkRobot(), validProfile());

    QVERIFY(result.ok());
    QCOMPARE(result.status(), KinematicsStatus::Ok);
    QCOMPARE(result.issues.size(), std::size_t(0));
}

void CollisionProfileValidatorTests::duplicateGeometryIdsAreRejected()
{
    CollisionProfile profile = validProfile();
    profile.geometries[1].id = profile.geometries[0].id;

    const CollisionProfileValidationResult result =
        CollisionProfileValidator::validate(threeLinkRobot(), profile);

    QVERIFY(!result.ok());
    QCOMPARE(result.status(), KinematicsStatus::InvalidRobotConfig);
    QVERIFY(QString::fromStdString(result.issues.front().message).contains("unique"));
}

void CollisionProfileValidatorTests::missingLinkIdsAreRejected()
{
    CollisionProfile profile = validProfile();
    profile.geometries[1].linkId = "missing_link";

    const CollisionProfileValidationResult result =
        CollisionProfileValidator::validate(threeLinkRobot(), profile);

    QVERIFY(!result.ok());
    QCOMPARE(result.status(), KinematicsStatus::InvalidRobotConfig);
    QVERIFY(QString::fromStdString(result.issues.front().field).contains(".linkId"));
}

void CollisionProfileValidatorTests::nonPositivePrimitiveDimensionsAreRejected()
{
    CollisionProfile sphereProfile = validProfile();
    sphereProfile.geometries[0].shape.sphere.radius_m = 0.0;

    CollisionProfile capsuleProfile = validProfile();
    capsuleProfile.geometries[1].shape.capsule.radius_m = -0.01;
    capsuleProfile.geometries[1].shape.capsule.length_m = 0.0;

    const CollisionProfileValidationResult sphereResult =
        CollisionProfileValidator::validate(threeLinkRobot(), sphereProfile);
    const CollisionProfileValidationResult capsuleResult =
        CollisionProfileValidator::validate(threeLinkRobot(), capsuleProfile);

    QVERIFY(!sphereResult.ok());
    QVERIFY(!capsuleResult.ok());
    QVERIFY(QString::fromStdString(sphereResult.issues.front().message).contains("positive"));
    QVERIFY(QString::fromStdString(capsuleResult.issues.front().message).contains("positive"));
}

void CollisionProfileValidatorTests::invalidDisabledPairsAreRejected()
{
    CollisionProfile profile = validProfile();
    profile.disabledPairs.push_back(
        DisabledCollisionPair{"base_body", "unknown_geometry", "typo_fixture"});

    const CollisionProfileValidationResult result =
        CollisionProfileValidator::validate(threeLinkRobot(), profile);

    QVERIFY(!result.ok());
    QCOMPARE(result.status(), KinematicsStatus::InvalidRobotConfig);
    QVERIFY(QString::fromStdString(result.issues.back().field).contains("disabledPairs"));
}

int runCollisionProfileValidatorTests(int argc, char** argv)
{
    CollisionProfileValidatorTests tests;
    return QTest::qExec(&tests, argc, argv);
}
