#include "StlPrimitiveAuthoringHelperTests.h"

#include <RobotKinematics/Collision/StlPrimitiveAuthoringHelper.h>

#include <Eigen/Geometry>

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QtTest/QtTest>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace RobotKinematics;

namespace {
struct Vec3
{
    double x;
    double y;
    double z;
};

struct Triangle
{
    Vec3 a;
    Vec3 b;
    Vec3 c;
};

constexpr double kPi = 3.14159265358979323846;
constexpr double kSqrt2Over2 = 0.7071067811865476;
constexpr double kExpectedSphereRadius = 1.224744871391589;
constexpr double kExpectedCapsuleRadius = 0.7071067811865476;
constexpr double kExpectedCapsuleLength = 0.5857864376269049;

std::vector<Triangle> cuboidTriangles()
{
    const Vec3 p000{0.0, 0.0, 0.0};
    const Vec3 p100{2.0, 0.0, 0.0};
    const Vec3 p010{0.0, 1.0, 0.0};
    const Vec3 p110{2.0, 1.0, 0.0};
    const Vec3 p001{0.0, 0.0, 1.0};
    const Vec3 p101{2.0, 0.0, 1.0};
    const Vec3 p011{0.0, 1.0, 1.0};
    const Vec3 p111{2.0, 1.0, 1.0};

    return {
        {p000, p110, p100}, {p000, p010, p110},
        {p001, p101, p111}, {p001, p111, p011},
        {p000, p100, p101}, {p000, p101, p001},
        {p010, p011, p111}, {p010, p111, p110},
        {p000, p001, p011}, {p000, p011, p010},
        {p100, p110, p111}, {p100, p111, p101},
    };
}

QByteArray asciiStlBytes()
{
    QByteArray bytes;
    bytes += "solid cuboid\n";
    for (const Triangle& triangle : cuboidTriangles()) {
        bytes += "  facet normal 0 0 0\n";
        bytes += "    outer loop\n";
        bytes += QByteArray("      vertex ") + QByteArray::number(triangle.a.x, 'f', 6) + " " +
                 QByteArray::number(triangle.a.y, 'f', 6) + " " +
                 QByteArray::number(triangle.a.z, 'f', 6) + "\n";
        bytes += QByteArray("      vertex ") + QByteArray::number(triangle.b.x, 'f', 6) + " " +
                 QByteArray::number(triangle.b.y, 'f', 6) + " " +
                 QByteArray::number(triangle.b.z, 'f', 6) + "\n";
        bytes += QByteArray("      vertex ") + QByteArray::number(triangle.c.x, 'f', 6) + " " +
                 QByteArray::number(triangle.c.y, 'f', 6) + " " +
                 QByteArray::number(triangle.c.z, 'f', 6) + "\n";
        bytes += "    endloop\n";
        bytes += "  endfacet\n";
    }
    bytes += "endsolid cuboid\n";
    return bytes;
}

void appendLeUInt32(QByteArray& bytes, std::uint32_t value)
{
    char raw[4];
    std::memcpy(raw, &value, sizeof(value));
    bytes.append(raw, static_cast<int>(sizeof(value)));
}

void appendLeFloat(QByteArray& bytes, float value)
{
    char raw[4];
    std::memcpy(raw, &value, sizeof(value));
    bytes.append(raw, static_cast<int>(sizeof(value)));
}

QByteArray binaryStlBytes()
{
    const std::vector<Triangle> triangles = cuboidTriangles();

    QByteArray bytes(80, '\0');
    const QByteArray header("cuboid-binary");
    std::copy(header.begin(), header.end(), bytes.begin());
    appendLeUInt32(bytes, static_cast<std::uint32_t>(triangles.size()));

    for (const Triangle& triangle : triangles) {
        appendLeFloat(bytes, 0.0f);
        appendLeFloat(bytes, 0.0f);
        appendLeFloat(bytes, 0.0f);

        const Vec3 vertices[] = {triangle.a, triangle.b, triangle.c};
        for (const Vec3& vertex : vertices) {
            appendLeFloat(bytes, static_cast<float>(vertex.x));
            appendLeFloat(bytes, static_cast<float>(vertex.y));
            appendLeFloat(bytes, static_cast<float>(vertex.z));
        }

        bytes.append('\0');
        bytes.append('\0');
    }

    return bytes;
}

QString writeTempFile(const QString& fileName, const QByteArray& bytes)
{
    const QString path = QDir::temp().filePath(fileName);
    QFile::remove(path);

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return QString();
    }
    if (file.write(bytes) != bytes.size()) {
        return QString();
    }
    file.close();
    return path;
}

void verifyCommonProposal(const StlPrimitiveProposal& proposal, StlFileFormat expectedFormat)
{
    QCOMPARE(static_cast<int>(proposal.format), static_cast<int>(expectedFormat));
    QCOMPARE(static_cast<qulonglong>(proposal.statistics.triangleCount), qulonglong(12));
    QCOMPARE(proposal.statistics.minimumBounds_m[0], 0.0);
    QCOMPARE(proposal.statistics.minimumBounds_m[1], 0.0);
    QCOMPARE(proposal.statistics.minimumBounds_m[2], 0.0);
    QCOMPARE(proposal.statistics.maximumBounds_m[0], 2.0);
    QCOMPARE(proposal.statistics.maximumBounds_m[1], 1.0);
    QCOMPARE(proposal.statistics.maximumBounds_m[2], 1.0);
    QCOMPARE(proposal.statistics.centroid_m[0], 1.0);
    QCOMPARE(proposal.statistics.centroid_m[1], 0.5);
    QCOMPARE(proposal.statistics.centroid_m[2], 0.5);
    QCOMPARE(proposal.statistics.axisLengths_m[0], 2.0);
    QCOMPARE(proposal.statistics.axisLengths_m[1], 1.0);
    QCOMPARE(proposal.statistics.axisLengths_m[2], 1.0);

    QCOMPARE(proposal.sphere.shape.type, CollisionShapeType::Sphere);
    QVERIFY(std::abs(proposal.sphere.shape.sphere.radius_m - kExpectedSphereRadius) <= 1e-12);
    QVERIFY((proposal.sphere.geometryToLink.translation_m() - Eigen::Vector3d(1.0, 0.5, 0.5)).norm() <= 1e-12);

    QCOMPARE(proposal.capsule.shape.type, CollisionShapeType::Capsule);
    QVERIFY(std::abs(proposal.capsule.shape.capsule.radius_m - kExpectedCapsuleRadius) <= 1e-12);
    QVERIFY(std::abs(proposal.capsule.shape.capsule.length_m - kExpectedCapsuleLength) <= 1e-12);
    QVERIFY((proposal.capsule.geometryToLink.translation_m() - Eigen::Vector3d(1.0, 0.5, 0.5)).norm() <= 1e-12);

    const Eigen::Vector3d axisInMesh =
        proposal.capsule.geometryToLink.isometry().linear() * Eigen::Vector3d::UnitZ();
    QVERIFY((axisInMesh - Eigen::Vector3d::UnitX()).norm() <= 1e-12);

    QVERIFY(proposal.sphereDraftJson.find("draft_manual_review_required") != std::string::npos);
    QVERIFY(proposal.capsuleDraftJson.find("draft_manual_review_required") != std::string::npos);
}
}

void StlPrimitiveAuthoringHelperTests::proposesConservativePrimitivesFromAsciiStl()
{
    const QString path = writeTempFile(QStringLiteral("rk_ascii_cuboid.stl"), asciiStlBytes());
    QVERIFY(!path.isEmpty());

    StlPrimitiveProposalRequest request;
    request.profileId = "draft_profile";
    request.robotModel = "DraftRobot";
    request.geometryId = "upper_arm";
    request.linkId = "link_2";
    request.margin_m = 0.01;

    const Result<StlPrimitiveProposal> result =
        StlPrimitiveAuthoringHelper::proposeFromFile(path.toStdString(), request);

    QVERIFY2(result.ok(), result.message.c_str());
    verifyCommonProposal(result.value, StlFileFormat::Ascii);
    QCOMPARE(result.value.sphere.id, std::string("upper_arm_sphere"));
    QCOMPARE(result.value.capsule.id, std::string("upper_arm_capsule"));
    QCOMPARE(result.value.sphere.linkId, std::string("link_2"));
    QCOMPARE(result.value.capsule.linkId, std::string("link_2"));
    QCOMPARE(result.value.sphere.margin_m, 0.01);
    QCOMPARE(result.value.capsule.margin_m, 0.01);
}

void StlPrimitiveAuthoringHelperTests::proposesConservativePrimitivesFromBinaryStl()
{
    const QString path = writeTempFile(QStringLiteral("rk_binary_cuboid.stl"), binaryStlBytes());
    QVERIFY(!path.isEmpty());

    const Result<StlPrimitiveProposal> result =
        StlPrimitiveAuthoringHelper::proposeFromFile(path.toStdString(), {});

    QVERIFY2(result.ok(), result.message.c_str());
    verifyCommonProposal(result.value, StlFileFormat::Binary);
}

void StlPrimitiveAuthoringHelperTests::rejectsInvalidStlPayload()
{
    const QString path = writeTempFile(QStringLiteral("rk_invalid_payload.stl"), QByteArray("not an stl"));
    QVERIFY(!path.isEmpty());

    const Result<StlPrimitiveProposal> result =
        StlPrimitiveAuthoringHelper::proposeFromFile(path.toStdString(), {});

    QVERIFY(!result.ok());
    QCOMPARE(result.status, KinematicsStatus::InvalidRequest);
}

int runStlPrimitiveAuthoringHelperTests(int argc, char** argv)
{
    StlPrimitiveAuthoringHelperTests tests;
    return QTest::qExec(&tests, argc, argv);
}
