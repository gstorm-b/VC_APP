#include <RobotKinematics/Collision/CollisionProfileJsonLoader.h>

#include <RobotKinematics/Core/Pose.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <set>

namespace RobotKinematics {

namespace {
Result<CollisionProfile> invalid(const std::string& message)
{
    return Result<CollisionProfile>::failure(KinematicsStatus::InvalidRobotConfig, message);
}

bool hasOnlyKnownTopLevelFields(const QJsonObject& object)
{
    const std::set<QString> known = {
        "schema", "profile", "geometries", "disabledPairs", "sources", "metadata",
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

Pose poseFromObject(const QJsonObject& object)
{
    const QJsonArray xyz = object.value("xyz_m").toArray();
    const QJsonArray rpy = object.value("rpy_rad").toArray();
    return Pose::fromXYZRPY_m_rad(xyz.at(0).toDouble(), xyz.at(1).toDouble(), xyz.at(2).toDouble(),
                                  rpy.at(0).toDouble(), rpy.at(1).toDouble(), rpy.at(2).toDouble());
}
}

Result<CollisionProfile> CollisionProfileJsonLoader::loadFile(const std::string& path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<CollisionProfile>::failure(KinematicsStatus::InvalidRequest,
                                                 "collision profile file could not be opened");
    }
    return loadJson(QString::fromUtf8(file.readAll()).toStdString());
}

Result<CollisionProfile> CollisionProfileJsonLoader::loadJson(const std::string& json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return invalid("collision profile JSON is not a valid object");
    }

    const QJsonObject root = document.object();
    if (!hasOnlyKnownTopLevelFields(root)) {
        return invalid("collision profile contains unknown top-level fields");
    }
    if (root.value("schema").toString() != "robot-kinematics-collision/v1") {
        return invalid("unsupported collision profile schema");
    }

    const QJsonObject profileObject = root.value("profile").toObject();
    const QJsonObject units = profileObject.value("units").toObject();
    if (units.value("length").toString() != "m" || units.value("angle").toString() != "rad") {
        return invalid("collision profile units must be m/rad");
    }

    CollisionProfile profile;
    profile.id = stringField(profileObject, "id");
    profile.robotModel = stringField(profileObject, "robotModel");

    for (const QJsonValue& geometryValue : root.value("geometries").toArray()) {
        const QJsonObject geometryObject = geometryValue.toObject();
        CollisionGeometry geometry;
        geometry.id = stringField(geometryObject, "id");
        geometry.linkId = stringField(geometryObject, "link");
        geometry.geometryToLink = poseFromObject(geometryObject.value("geometryToLink").toObject());
        geometry.margin_m = geometryObject.value("margin_m").toDouble();
        geometry.enabled = !geometryObject.contains("enabled") || geometryObject.value("enabled").toBool();

        const QString shape = geometryObject.value("shape").toString();
        if (shape == "sphere") {
            geometry.shape.type = CollisionShapeType::Sphere;
            geometry.shape.sphere.radius_m =
                geometryObject.value("sphere").toObject().value("radius_m").toDouble();
        } else if (shape == "capsule") {
            geometry.shape.type = CollisionShapeType::Capsule;
            const QJsonObject capsule = geometryObject.value("capsule").toObject();
            geometry.shape.capsule.radius_m = capsule.value("radius_m").toDouble();
            geometry.shape.capsule.length_m = capsule.value("length_m").toDouble();
        } else {
            return invalid("collision profile shape must be sphere or capsule");
        }

        profile.geometries.push_back(geometry);
    }

    for (const QJsonValue& pairValue : root.value("disabledPairs").toArray()) {
        const QJsonObject pairObject = pairValue.toObject();
        profile.disabledPairs.push_back(DisabledCollisionPair{
            stringField(pairObject, "a"),
            stringField(pairObject, "b"),
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

    return Result<CollisionProfile>::success(profile);
}

} // namespace RobotKinematics
