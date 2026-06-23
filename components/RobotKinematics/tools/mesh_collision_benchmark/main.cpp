#include <RobotKinematics/Collision/CollisionBackend.h>
#include <RobotKinematics/Collision/MeshCollisionProfileJsonLoader.h>
#include <RobotKinematics/Presets/PresetJsonLoader.h>

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QStringList>

#include <Eigen/Core>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <optional>
#include <vector>

using namespace RobotKinematics;

namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;

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

struct ScenarioDefinition
{
    std::string label;
    SerialRobotConfig robot;
    MeshCollisionProfile profile;
    JointVector joints;
};

struct ScenarioMeasurement
{
    std::string label;
    KinematicsStatus status = KinematicsStatus::Ok;
    bool hasCollision = false;
    std::size_t pairCount = 0;
    std::size_t contactCount = 0;
    double distance_m = 0.0;
    qint64 totalNs = 0;
    int iterations = 0;
    int warmupIterations = 0;
};

Joint revoluteZ(const std::string& id,
                const std::string& parent,
                const std::string& child,
                const Pose& origin)
{
    Joint joint;
    joint.id = id;
    joint.type = JointType::Revolute;
    joint.parentLinkId = parent;
    joint.childLinkId = child;
    joint.axis = Eigen::Vector3d::UnitZ();
    joint.origin = origin;
    joint.limits = JointLimits{-kPi, kPi, std::nullopt, std::nullopt};
    joint.home = 0.0;
    return joint;
}

SerialRobotConfig lineRobot()
{
    SerialRobotConfig config;
    config.identity = RobotIdentity{"RobotKinematics", "CollisionLineRobot", "Collision Line Robot", "1.0.0"};
    config.topology = RobotTopologyType::Serial;
    config.dof = 3;
    config.links = {{"base_link"}, {"link_1"}, {"link_2"}, {"flange"}};
    config.joints = {
        revoluteZ("J1", "base_link", "link_1", Pose::identity()),
        revoluteZ("J2", "link_1", "link_2", Pose::fromXYZRPY_m_rad(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)),
        revoluteZ("J3", "link_2", "flange", Pose::fromXYZRPY_m_rad(1.0, 0.0, 0.0, 0.0, 0.0, 0.0)),
    };
    config.frames.baseLinkId = "base_link";
    config.frames.flangeLinkId = "flange";
    config.tools = {Tool{"default", "Default Tool", Pose::identity()}};
    config.defaultToolId = "default";
    return config;
}

std::vector<Triangle> cuboidTrianglesMeters()
{
    const Vec3 p000{0.0, 0.0, 0.0};
    const Vec3 p100{0.2, 0.0, 0.0};
    const Vec3 p010{0.0, 0.1, 0.0};
    const Vec3 p110{0.2, 0.1, 0.0};
    const Vec3 p001{0.0, 0.0, 0.1};
    const Vec3 p101{0.2, 0.0, 0.1};
    const Vec3 p011{0.0, 0.1, 0.1};
    const Vec3 p111{0.2, 0.1, 0.1};

    return {
        {p000, p110, p100}, {p000, p010, p110},
        {p001, p101, p111}, {p001, p111, p011},
        {p000, p100, p101}, {p000, p101, p001},
        {p010, p011, p111}, {p010, p111, p110},
        {p000, p001, p011}, {p000, p011, p010},
        {p100, p110, p111}, {p100, p111, p101},
    };
}

QByteArray asciiStlBytesMeters()
{
    QByteArray bytes;
    bytes += "solid cuboid\n";
    for (const Triangle& triangle : cuboidTrianglesMeters()) {
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

MeshCollisionProfile syntheticMeshProfile(const std::string& path)
{
    MeshCollisionProfile profile;
    profile.id = "synthetic_coal_profile";
    profile.robotModel = "CollisionLineRobot";
    profile.backendPreference = {MeshCollisionBackendKind::Coal};

    MeshCollisionGeometry baseMesh;
    baseMesh.id = "base_mesh";
    baseMesh.linkId = "base_link";
    baseMesh.path = path;
    baseMesh.format = MeshFileFormat::Stl;
    baseMesh.sourceUnits = MeshSourceUnits::Meters;
    baseMesh.scaleToMeters = 1.0;
    baseMesh.meshToLink = Pose::fromXYZRPY_m_rad(0.85, 0.0, 0.0, 0.0, 0.0, 0.0);

    MeshCollisionGeometry linkMesh;
    linkMesh.id = "link_2_mesh";
    linkMesh.linkId = "link_2";
    linkMesh.path = path;
    linkMesh.format = MeshFileFormat::Stl;
    linkMesh.sourceUnits = MeshSourceUnits::Meters;
    linkMesh.scaleToMeters = 1.0;

    profile.meshes = {baseMesh, linkMesh};
    return profile;
}

Result<std::vector<double>> parseJointCsv(const QString& csv)
{
    const QStringList tokens = csv.split(',', Qt::SkipEmptyParts);
    std::vector<double> values;
    values.reserve(static_cast<std::size_t>(tokens.size()));

    for (const QString& token : tokens) {
        bool ok = false;
        const double value = token.trimmed().toDouble(&ok);
        if (!ok || !std::isfinite(value)) {
            return Result<std::vector<double>>::failure(
                KinematicsStatus::InvalidRequest,
                "failed to parse joints-rad value: " + token.toStdString());
        }
        values.push_back(value);
    }

    return Result<std::vector<double>>::success(values);
}

Result<ScenarioDefinition> buildSyntheticScenario(const std::string& label, const std::vector<double>& joints)
{
    const QString meshPath =
        writeTempFile(QStringLiteral("rk_mesh_collision_benchmark_synthetic.stl"), asciiStlBytesMeters());
    if (meshPath.isEmpty()) {
        return Result<ScenarioDefinition>::failure(KinematicsStatus::InvalidRequest,
                                                   "failed to write synthetic STL fixture");
    }

    ScenarioDefinition scenario;
    scenario.label = label;
    scenario.robot = lineRobot();
    scenario.profile = syntheticMeshProfile(meshPath.toStdString());
    scenario.joints = JointVector::fromRadians(joints);
    return Result<ScenarioDefinition>::success(std::move(scenario));
}

Result<ScenarioDefinition> loadJsonScenario(const QString& label,
                                            const QString& presetPath,
                                            const QString& meshProfilePath,
                                            const QString& jointsCsv)
{
    const Result<SerialRobotConfig> robot = PresetJsonLoader::loadFile(presetPath.toStdString());
    if (!robot.ok()) {
        return Result<ScenarioDefinition>::failure(
            robot.status,
            "failed to load preset JSON: " + robot.message);
    }

    const Result<MeshCollisionProfile> profile =
        MeshCollisionProfileJsonLoader::loadFile(meshProfilePath.toStdString());
    if (!profile.ok()) {
        return Result<ScenarioDefinition>::failure(
            profile.status,
            "failed to load mesh profile JSON: " + profile.message);
    }

    const Result<std::vector<double>> joints = parseJointCsv(jointsCsv);
    if (!joints.ok()) {
        return Result<ScenarioDefinition>::failure(joints.status, joints.message);
    }

    ScenarioDefinition scenario;
    scenario.label = label.toStdString();
    scenario.robot = robot.value;
    scenario.profile = profile.value;
    scenario.joints = JointVector::fromRadians(joints.value);
    return Result<ScenarioDefinition>::success(std::move(scenario));
}

ScenarioMeasurement measureScenario(const ScenarioDefinition& scenario,
                                    const int iterations,
                                    const int warmupIterations)
{
    MeshCollisionCheckRequest request;
    request.joints = scenario.joints;

    for (int index = 0; index < warmupIterations; ++index) {
        const CollisionCheckResult warmup = CollisionBackends::checkMesh(scenario.robot, scenario.profile, request);
        if (!warmup.ok()) {
            return ScenarioMeasurement{
                scenario.label,
                warmup.status,
                warmup.hasCollision,
                warmup.pairs.size(),
                warmup.pairs.empty() ? std::size_t(0) : warmup.pairs.front().contactCount,
                warmup.pairs.empty() ? 0.0 : warmup.pairs.front().distance_m,
                0,
                0,
                warmupIterations,
            };
        }
    }

    CollisionCheckResult lastResult;
    QElapsedTimer timer;
    timer.start();
    for (int index = 0; index < iterations; ++index) {
        lastResult = CollisionBackends::checkMesh(scenario.robot, scenario.profile, request);
        if (!lastResult.ok()) {
            break;
        }
    }

    return ScenarioMeasurement{
        scenario.label,
        lastResult.status,
        lastResult.hasCollision,
        lastResult.pairs.size(),
        lastResult.pairs.empty() ? std::size_t(0) : lastResult.pairs.front().contactCount,
        lastResult.pairs.empty() ? 0.0 : lastResult.pairs.front().distance_m,
        timer.nsecsElapsed(),
        iterations,
        warmupIterations,
    };
}

void printMeasurement(const ScenarioMeasurement& measurement)
{
    const double totalMs = static_cast<double>(measurement.totalNs) / 1.0e6;
    const double avgUs = measurement.iterations > 0
                             ? (static_cast<double>(measurement.totalNs) / static_cast<double>(measurement.iterations)) / 1.0e3
                             : 0.0;

    std::cout << "scenario=" << measurement.label
              << " status=" << static_cast<int>(measurement.status)
              << " iterations=" << measurement.iterations
              << " warmup=" << measurement.warmupIterations
              << " hasCollision=" << measurement.hasCollision
              << " pairCount=" << measurement.pairCount
              << " contactCount=" << measurement.contactCount
              << " distance_m=" << measurement.distance_m
              << " total_ms=" << totalMs
              << " avg_us=" << avgUs
              << std::endl;
}
}

int main(int argc, char* argv[])
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    parser.setApplicationDescription("RobotKinematics mesh collision benchmark");
    parser.addHelpOption();

    const QCommandLineOption iterationsOption(
        QStringList{"i", "iterations"},
        "Measured iterations per scenario.",
        "count",
        "1000");
    const QCommandLineOption warmupOption(
        QStringList{"w", "warmup"},
        "Warm-up iterations per scenario.",
        "count",
        "100");
    const QCommandLineOption presetJsonOption(
        QStringList{"preset-json"},
        "Path to a robot preset JSON file for file-driven benchmarking.",
        "path");
    const QCommandLineOption meshProfileJsonOption(
        QStringList{"mesh-profile-json"},
        "Path to a mesh collision profile JSON file for file-driven benchmarking.",
        "path");
    const QCommandLineOption jointsRadOption(
        QStringList{"joints-rad"},
        "Comma-separated joint vector in radians for file-driven benchmarking.",
        "csv");
    const QCommandLineOption dumpPairsOption(
        QStringList{"dump-pairs"},
        "After running, dump per-pair distance/colliding info for the final iteration.");

    parser.addOption(iterationsOption);
    parser.addOption(warmupOption);
    parser.addOption(presetJsonOption);
    parser.addOption(meshProfileJsonOption);
    parser.addOption(jointsRadOption);
    parser.addOption(dumpPairsOption);
    parser.process(app);

    bool iterationsOk = false;
    const int iterations = parser.value(iterationsOption).toInt(&iterationsOk);
    bool warmupOk = false;
    const int warmupIterations = parser.value(warmupOption).toInt(&warmupOk);
    if (!iterationsOk || iterations <= 0 || !warmupOk || warmupIterations < 0) {
        std::cerr << "[ERROR] iterations must be > 0 and warmup must be >= 0." << std::endl;
        return 1;
    }

    std::vector<ScenarioDefinition> scenarios;

    const bool hasFileScenarioOptions =
        parser.isSet(presetJsonOption) || parser.isSet(meshProfileJsonOption) || parser.isSet(jointsRadOption);
    if (hasFileScenarioOptions) {
        if (!parser.isSet(presetJsonOption) || !parser.isSet(meshProfileJsonOption) || !parser.isSet(jointsRadOption)) {
            std::cerr << "[ERROR] preset-json, mesh-profile-json, and joints-rad must be provided together." << std::endl;
            return 1;
        }

        const Result<ScenarioDefinition> scenario =
            loadJsonScenario("file_json_case",
                             parser.value(presetJsonOption),
                             parser.value(meshProfileJsonOption),
                             parser.value(jointsRadOption));
        if (!scenario.ok()) {
            std::cerr << "[ERROR] " << scenario.message << std::endl;
            return 1;
        }
        scenarios.push_back(scenario.value);
    } else {
        const Result<ScenarioDefinition> overlapScenario =
            buildSyntheticScenario("synthetic_overlap", {0.0, 0.0, 0.0});
        if (!overlapScenario.ok()) {
            std::cerr << "[ERROR] " << overlapScenario.message << std::endl;
            return 1;
        }

        const Result<ScenarioDefinition> separatedScenario =
            buildSyntheticScenario("synthetic_separated", {kPi * 0.5, 0.0, 0.0});
        if (!separatedScenario.ok()) {
            std::cerr << "[ERROR] " << separatedScenario.message << std::endl;
            return 1;
        }

        scenarios.push_back(overlapScenario.value);
        scenarios.push_back(separatedScenario.value);
    }

    for (const ScenarioDefinition& scenario : scenarios) {
        const ScenarioMeasurement measurement = measureScenario(scenario, iterations, warmupIterations);
        printMeasurement(measurement);
        if (measurement.status != KinematicsStatus::Ok) {
            std::cerr << "[ERROR] benchmark scenario failed before measurement completed: "
                      << measurement.label << std::endl;
            return 1;
        }
        if (parser.isSet(dumpPairsOption)) {
            MeshCollisionCheckRequest request;
            request.joints = scenario.joints;
            request.returnAllPairs = true;
            const CollisionCheckResult result =
                CollisionBackends::checkMesh(scenario.robot, scenario.profile, request);
            for (const CollisionPairResult& pair : result.pairs) {
                std::cout << "pair scenario=" << scenario.label
                          << " a=" << pair.geometryA
                          << " b=" << pair.geometryB
                          << " colliding=" << pair.colliding
                          << " distance_m=" << pair.distance_m
                          << " contactCount=" << pair.contactCount
                          << std::endl;
            }
        }
    }

    return 0;
}
