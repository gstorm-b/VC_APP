#include <RobotKinematics/Presets/PresetJsonLoader.h>

#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Model/RobotModelValidator.h>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include <set>

namespace RobotKinematics {

namespace {
Result<SerialRobotConfig> invalid(const std::string& message)
{
    return Result<SerialRobotConfig>::failure(KinematicsStatus::InvalidRobotConfig, message);
}

bool hasOnlyKnownTopLevelFields(const QJsonObject& object)
{
    const std::set<QString> known = {
        "schema", "identity", "units", "topology", "links", "joints", "frames",
        "tools", "defaultTool", "posture", "solver", "sources", "metadata",
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

JointType jointTypeFromString(const QString& value)
{
    if (value == "prismatic") {
        return JointType::Prismatic;
    }
    if (value == "fixed") {
        return JointType::Fixed;
    }
    return JointType::Revolute;
}

Joint parseJoint(const QJsonObject& object)
{
    Joint joint;
    joint.id = stringField(object, "id");
    joint.type = jointTypeFromString(object.value("type").toString());
    joint.parentLinkId = stringField(object, "parent");
    joint.childLinkId = stringField(object, "child");
    joint.origin = poseFromObject(object.value("origin").toObject());

    const QJsonArray axis = object.value("axis").toArray();
    joint.axis = Eigen::Vector3d(axis.at(0).toDouble(), axis.at(1).toDouble(), axis.at(2).toDouble());

    if (object.contains("limits") && object.value("limits").isObject()) {
        const QJsonObject limits = object.value("limits").toObject();
        JointLimits parsed;
        parsed.lower = limits.value("lower").toDouble();
        parsed.upper = limits.value("upper").toDouble();
        if (limits.contains("velocity") && !limits.value("velocity").isNull()) {
            parsed.velocity = limits.value("velocity").toDouble();
        }
        if (limits.contains("acceleration") && !limits.value("acceleration").isNull()) {
            parsed.acceleration = limits.value("acceleration").toDouble();
        }
        joint.limits = parsed;
    }
    joint.home = object.value("home").toDouble();
    for (const QJsonValue& alias : object.value("aliases").toArray()) {
        joint.aliases.push_back(alias.toString().toStdString());
    }
    return joint;
}

void parsePosture(const QJsonObject& object, SerialRobotConfig& config)
{
    config.posture.resolver = stringField(object, "resolver");
    const QJsonObject labels = object.value("labels").toObject();
    for (auto it = labels.begin(); it != labels.end(); ++it) {
        const QJsonObject axis = it.value().toObject();
        config.posture.labels[it.key().toStdString()] =
            PostureLabelAxis{stringField(axis, "negative"), stringField(axis, "positive")};
    }
}
}

Result<SerialRobotConfig> PresetJsonLoader::loadFile(const std::string& path)
{
    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return Result<SerialRobotConfig>::failure(KinematicsStatus::InvalidRequest,
                                                  "preset file could not be opened");
    }
    return loadJson(QString::fromUtf8(file.readAll()).toStdString());
}

Result<SerialRobotConfig> PresetJsonLoader::loadJson(const std::string& json)
{
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        return invalid("preset JSON is not a valid object");
    }

    const QJsonObject root = document.object();
    if (!hasOnlyKnownTopLevelFields(root)) {
        return invalid("preset contains unknown top-level fields");
    }
    if (root.value("schema").toString() != "robot-kinematics-preset/v1") {
        return invalid("unsupported preset schema");
    }
    const QJsonObject units = root.value("units").toObject();
    if (units.value("length").toString() != "m" || units.value("angle").toString() != "rad") {
        return invalid("preset units must be m/rad");
    }

    SerialRobotConfig config;
    const QJsonObject identity = root.value("identity").toObject();
    config.identity = RobotIdentity{
        stringField(identity, "vendor"),
        stringField(identity, "model"),
        stringField(identity, "name"),
        stringField(identity, "revision"),
    };

    const QJsonObject topology = root.value("topology").toObject();
    config.topology = RobotTopologyType::Serial;
    config.dof = topology.value("dof").toInt();

    for (const QJsonValue& link : root.value("links").toArray()) {
        config.links.push_back(Link{stringField(link.toObject(), "id")});
    }
    for (const QJsonValue& joint : root.value("joints").toArray()) {
        config.joints.push_back(parseJoint(joint.toObject()));
    }

    const QJsonObject frames = root.value("frames").toObject();
    config.frames.baseLinkId = stringField(frames, "base");
    config.frames.flangeLinkId = stringField(frames, "flange");
    for (const QJsonValue& frameValue : frames.value("userFrames").toArray()) {
        const QJsonObject frame = frameValue.toObject();
        config.frames.userFrames.push_back(UserFrame{
            stringField(frame, "id"),
            stringField(frame, "parent"),
            poseFromObject(frame.value("transform").toObject()),
        });
    }

    for (const QJsonValue& toolValue : root.value("tools").toArray()) {
        const QJsonObject tool = toolValue.toObject();
        config.tools.push_back(Tool{
            stringField(tool, "id"),
            stringField(tool, "name"),
            poseFromObject(tool.value("flangeToTcp").toObject()),
        });
    }
    config.defaultToolId = root.value("defaultTool").toString().toStdString();

    parsePosture(root.value("posture").toObject(), config);

    const QJsonObject solver = root.value("solver").toObject();
    config.solver.defaultSolver = stringField(solver, "default");
    const QJsonObject parameters = solver.value("parameters").toObject();
    for (auto it = parameters.begin(); it != parameters.end(); ++it) {
        if (it.value().isDouble()) {
            config.solver.numericParameters[it.key().toStdString()] = it.value().toDouble();
        }
    }

    for (const QJsonValue& sourceValue : root.value("sources").toArray()) {
        const QJsonObject source = sourceValue.toObject();
        SourceReference parsed;
        parsed.type = stringField(source, "type");
        parsed.title = stringField(source, "title");
        parsed.reference = stringField(source, "reference");
        for (const QJsonValue& appliesTo : source.value("appliesTo").toArray()) {
            parsed.appliesTo.push_back(appliesTo.toString().toStdString());
        }
        config.sources.push_back(parsed);
    }

    const QJsonObject metadata = root.value("metadata").toObject();
    for (auto it = metadata.begin(); it != metadata.end(); ++it) {
        if (it.value().isString()) {
            config.metadata[it.key().toStdString()] = it.value().toString().toStdString();
        }
    }

    const ModelValidationResult validation = RobotModelValidator::validateSerialRobotConfig(config);
    if (!validation.ok()) {
        return invalid(validation.issues.front().message);
    }
    return Result<SerialRobotConfig>::success(config);
}

} // namespace RobotKinematics
