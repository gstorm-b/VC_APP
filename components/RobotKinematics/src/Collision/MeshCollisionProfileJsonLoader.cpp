#include <RobotKinematics/Collision/MeshCollisionProfileJsonLoader.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <cmath>
#include <limits>
#include <set>

namespace RobotKinematics {

namespace {
Result<MeshCollisionProfile> invalid(const std::string& message)
{
    return Result<MeshCollisionProfile>::failure(KinematicsStatus::InvalidRobotConfig, message);
}

bool hasOnlyKnownTopLevelFields(const QJsonObject& object)
{
    const std::set<QString> known = {
        "schema", "profile", "meshes", "disabledPairs", "sources", "metadata",
    };
    for (auto it = object.begin(); it != object.end(); ++it) {
        if (known.find(it.key()) == known.end()) {
            return false;
        }
    }
    return true;
}

std::string stringField(const QJsonObject& object, const char* key)
{
    return object.value(key).toString().toStdString();
}

bool isFiniteNumber(const QJsonValue& value)
{
    return value.isDouble() && std::isfinite(value.toDouble());
}

bool hasFiniteNumericField(const QJsonObject& object, const char* key)
{
    return object.contains(key) && isFiniteNumber(object.value(key));
}

bool hasFiniteNumericArray(const QJsonObject& object, const char* key)
{
    const QJsonArray array = object.value(key).toArray();
    if (array.size() != 3) {
        return false;
    }
    for (const QJsonValue& value : array) {
        if (!isFiniteNumber(value)) {
            return false;
        }
    }
    return true;
}

Pose poseFromObject(const QJsonObject& object)
{
    const QJsonArray xyz = object.value("xyz_m").toArray();
    const QJsonArray rpy = object.value("rpy_rad").toArray();
    return Pose::fromXYZRPY_m_rad(xyz.at(0).toDouble(), xyz.at(1).toDouble(), xyz.at(2).toDouble(),
                                  rpy.at(0).toDouble(), rpy.at(1).toDouble(), rpy.at(2).toDouble());
}

Result<MeshSourceUnits> parseSourceUnits(const QString& value)
{
    if (value == "m") {
        return Result<MeshSourceUnits>::success(MeshSourceUnits::Meters);
    }
    if (value == "mm") {
        return Result<MeshSourceUnits>::success(MeshSourceUnits::Millimeters);
    }
    return Result<MeshSourceUnits>::failure(KinematicsStatus::InvalidRobotConfig,
                                            "mesh sourceUnits must be 'm' or 'mm'");
}

Result<MeshFileFormat> parseFormat(const QString& value)
{
    if (value == "stl") {
        return Result<MeshFileFormat>::success(MeshFileFormat::Stl);
    }
    return Result<MeshFileFormat>::failure(KinematicsStatus::InvalidRobotConfig,
                                           "mesh format must be 'stl'");
}

Result<MeshQualityMode> parseQualityMode(const QString& value)
{
    if (value == "original") {
        return Result<MeshQualityMode>::success(MeshQualityMode::Original);
    }
    if (value == "simplified") {
        return Result<MeshQualityMode>::success(MeshQualityMode::Simplified);
    }
    if (value == "convex") {
        return Result<MeshQualityMode>::success(MeshQualityMode::Convex);
    }
    return Result<MeshQualityMode>::failure(KinematicsStatus::InvalidRobotConfig,
                                            "mesh quality mode must be original, simplified, or convex");
}
}

Result<MeshCollisionProfile> MeshCollisionProfileJsonLoader::loadFile(const std::string& path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<MeshCollisionProfile>::failure(KinematicsStatus::InvalidRequest,
                                                     "mesh collision profile file could not be opened");
    }

    Result<MeshCollisionProfile> loaded = loadJson(QString::fromUtf8(file.readAll()).toStdString());
    if (!loaded.ok()) {
        return loaded;
    }

    const QDir baseDir = QFileInfo(QString::fromStdString(path)).absoluteDir();
    for (MeshCollisionGeometry& mesh : loaded.value.meshes) {
        QFileInfo meshPath(QString::fromStdString(mesh.path));
        if (!meshPath.isAbsolute()) {
            mesh.path = QDir::cleanPath(baseDir.absoluteFilePath(meshPath.filePath())).toStdString();
        }
    }

    return loaded;
}

Result<MeshCollisionProfile> MeshCollisionProfileJsonLoader::loadJson(const std::string& json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return invalid("mesh collision profile JSON is not a valid object");
    }

    const QJsonObject root = document.object();
    if (!hasOnlyKnownTopLevelFields(root)) {
        return invalid("mesh collision profile contains unknown top-level fields");
    }
    if (root.value("schema").toString() != "robot-kinematics-collision-mesh/v1") {
        return invalid("unsupported mesh collision profile schema");
    }

    const QJsonObject profileObject = root.value("profile").toObject();
    const QJsonObject units = profileObject.value("units").toObject();
    if (units.value("length").toString() != "m" || units.value("angle").toString() != "rad") {
        return invalid("mesh collision profile units must be m/rad");
    }

    MeshCollisionProfile profile;
    profile.id = stringField(profileObject, "id");
    profile.robotModel = stringField(profileObject, "robotModel");

    for (const QJsonValue& backendValue : profileObject.value("backendPreference").toArray()) {
        const Result<MeshCollisionBackendKind> backend =
            meshCollisionBackendKindFromString(backendValue.toString().toStdString());
        if (!backend.ok()) {
            return invalid(backend.message);
        }
        profile.backendPreference.push_back(backend.value);
    }

    std::set<std::string> meshIds;
    for (const QJsonValue& meshValue : root.value("meshes").toArray()) {
        const QJsonObject meshObject = meshValue.toObject();

        if (!meshObject.contains("meshToLink")) {
            return invalid("mesh collision geometry must include meshToLink");
        }
        const QJsonObject meshToLinkObject = meshObject.value("meshToLink").toObject();
        if (!hasFiniteNumericArray(meshToLinkObject, "xyz_m") ||
            !hasFiniteNumericArray(meshToLinkObject, "rpy_rad")) {
            return invalid("mesh collision geometry meshToLink must include xyz_m and rpy_rad arrays");
        }
        if (!hasFiniteNumericField(meshObject, "scaleToMeters")) {
            return invalid("mesh collision geometry scaleToMeters must be a finite number");
        }
        if (meshObject.contains("margin_m") && !isFiniteNumber(meshObject.value("margin_m"))) {
            return invalid("mesh collision geometry margin_m must be a finite number");
        }
        if (meshObject.contains("enabled") && !meshObject.value("enabled").isBool()) {
            return invalid("mesh collision geometry enabled must be a boolean");
        }

        MeshCollisionGeometry mesh;
        mesh.id = stringField(meshObject, "id");
        mesh.linkId = stringField(meshObject, "link");
        mesh.path = stringField(meshObject, "path");
        mesh.scaleToMeters = meshObject.value("scaleToMeters").toDouble();
        mesh.meshToLink = poseFromObject(meshToLinkObject);
        mesh.margin_m = meshObject.value("margin_m").toDouble();
        mesh.enabled = !meshObject.contains("enabled") || meshObject.value("enabled").toBool();

        const Result<MeshFileFormat> format =
            parseFormat(meshObject.value("format").toString());
        if (!format.ok()) {
            return invalid(format.message);
        }
        mesh.format = format.value;

        const Result<MeshSourceUnits> sourceUnits =
            parseSourceUnits(meshObject.value("sourceUnits").toString());
        if (!sourceUnits.ok()) {
            return invalid(sourceUnits.message);
        }
        mesh.sourceUnits = sourceUnits.value;

        const QJsonObject qualityObject = meshObject.value("quality").toObject();
        const Result<MeshQualityMode> qualityMode =
            parseQualityMode(qualityObject.value("mode").toString());
        if (!qualityMode.ok()) {
            return invalid(qualityMode.message);
        }
        mesh.quality.mode = qualityMode.value;
        if (qualityObject.contains("triangleCount") && !qualityObject.value("triangleCount").isNull()) {
            const QJsonValue triangleCount = qualityObject.value("triangleCount");
            const double rawCount = triangleCount.toDouble(-1.0);
            if (!triangleCount.isDouble() || !std::isfinite(rawCount) || rawCount < 1.0 ||
                std::floor(rawCount) != rawCount ||
                rawCount > static_cast<double>((std::numeric_limits<std::size_t>::max)())) {
                return invalid("mesh quality triangleCount must be a positive integer when provided");
            }
            mesh.quality.triangleCount = static_cast<std::size_t>(rawCount);
        }
        if (qualityObject.contains("simplifiedFrom") && !qualityObject.value("simplifiedFrom").isNull()) {
            if (!qualityObject.value("simplifiedFrom").isString()) {
                return invalid("mesh quality simplifiedFrom must be a string when provided");
            }
            mesh.quality.simplifiedFrom = qualityObject.value("simplifiedFrom").toString().toStdString();
        }
        if (qualityObject.contains("maxSimplificationError_m") &&
            !qualityObject.value("maxSimplificationError_m").isNull()) {
            if (!isFiniteNumber(qualityObject.value("maxSimplificationError_m"))) {
                return invalid("mesh quality maxSimplificationError_m must be a finite number when provided");
            }
            mesh.quality.maxSimplificationError_m =
                qualityObject.value("maxSimplificationError_m").toDouble();
        }

        if (!meshIds.insert(mesh.id).second) {
            return invalid("mesh collision geometry ids must be unique");
        }

        profile.meshes.push_back(mesh);
    }

    for (const QJsonValue& pairValue : root.value("disabledPairs").toArray()) {
        const QJsonObject pairObject = pairValue.toObject();
        const std::string geometryA = stringField(pairObject, "a");
        const std::string geometryB = stringField(pairObject, "b");

        if (meshIds.find(geometryA) == meshIds.end() || meshIds.find(geometryB) == meshIds.end()) {
            return invalid("mesh collision disabledPairs must refer to existing mesh ids");
        }

        profile.disabledPairs.push_back(DisabledCollisionPair{
            geometryA,
            geometryB,
            stringField(pairObject, "reason"),
        });
    }

    for (const QJsonValue& sourceValue : root.value("sources").toArray()) {
        const QJsonObject sourceObject = sourceValue.toObject();
        SourceReference source;
        source.type = stringField(sourceObject, "type");
        source.title = stringField(sourceObject, "title");
        source.reference = stringField(sourceObject, "reference");
        for (const QJsonValue& appliesTo : sourceObject.value("appliesTo").toArray()) {
            source.appliesTo.push_back(appliesTo.toString().toStdString());
        }
        profile.sources.push_back(source);
    }

    const QJsonObject metadata = root.value("metadata").toObject();
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        if (it.value().isString()) {
            profile.metadata[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    return Result<MeshCollisionProfile>::success(profile);
}

} // namespace RobotKinematics
