#include <RobotKinematics/Adapters/UrdfAdapter.h>

#include <RobotKinematics/Core/Pose.h>
#include <RobotKinematics/Model/RobotModelValidator.h>

#include <QXmlStreamReader>

#include <Eigen/Geometry>

#include <QStringList>

#include <algorithm>
#include <cmath>
#include <map>
#include <sstream>

namespace RobotKinematics {

namespace {
std::string jointTypeName(JointType type)
{
    if (type == JointType::Prismatic) {
        return "prismatic";
    }
    if (type == JointType::Fixed) {
        return "fixed";
    }
    return "revolute";
}

JointType jointTypeFromName(const QString& type)
{
    if (type == "prismatic") {
        return JointType::Prismatic;
    }
    if (type == "fixed") {
        return JointType::Fixed;
    }
    return JointType::Revolute;
}

Eigen::Vector3d rpyFromPose(const Pose& pose)
{
    const Eigen::Matrix3d r = pose.isometry().linear();
    const double pitch = std::asin(std::max(-1.0, std::min(1.0, -r(2, 0))));
    const double roll = std::atan2(r(2, 1), r(2, 2));
    const double yaw = std::atan2(r(1, 0), r(0, 0));
    return Eigen::Vector3d(roll, pitch, yaw);
}

std::string vec3(const Eigen::Vector3d& v)
{
    std::ostringstream out;
    out << v.x() << " " << v.y() << " " << v.z();
    return out.str();
}

Eigen::Vector3d parseVec3(const QString& value, const Eigen::Vector3d& fallback)
{
    const QStringList parts = value.split(' ', Qt::SkipEmptyParts);
    if (parts.size() != 3) {
        return fallback;
    }
    return Eigen::Vector3d(parts[0].toDouble(), parts[1].toDouble(), parts[2].toDouble());
}
}

Result<std::string> UrdfAdapter::exportSerialRobot(const SerialRobotConfig& config)
{
    const ModelValidationResult validation = RobotModelValidator::validateSerialRobotConfig(config);
    if (!validation.ok()) {
        return Result<std::string>::failure(validation.status(), validation.issues.front().message);
    }

    std::ostringstream xml;
    xml << "<robot name=\"" << config.identity.model << "\">\n";
    for (const Link& link : config.links) {
        xml << "  <link name=\"" << link.id << "\"/>\n";
    }
    for (const Joint& joint : config.joints) {
        const Eigen::Vector3d rpy = rpyFromPose(joint.origin);
        xml << "  <joint name=\"" << joint.id << "\" type=\"" << jointTypeName(joint.type) << "\">\n";
        xml << "    <parent link=\"" << joint.parentLinkId << "\"/>\n";
        xml << "    <child link=\"" << joint.childLinkId << "\"/>\n";
        xml << "    <origin xyz=\"" << vec3(joint.origin.translation_m()) << "\" rpy=\"" << vec3(rpy) << "\"/>\n";
        if (joint.type != JointType::Fixed) {
            xml << "    <axis xyz=\"" << vec3(joint.axis) << "\"/>\n";
            if (joint.limits.has_value()) {
                xml << "    <limit lower=\"" << joint.limits->lower << "\" upper=\"" << joint.limits->upper << "\"/>\n";
            }
        }
        xml << "  </joint>\n";
    }
    if (!config.posture.resolver.empty() || !config.tools.empty() || !config.sources.empty()) {
        xml << "  <!-- RobotKinematics metadata not represented in URDF: tools, posture, solver, sources. -->\n";
    }
    xml << "</robot>\n";
    return Result<std::string>::success(xml.str());
}

Result<SerialRobotConfig> UrdfAdapter::importSerialRobot(const std::string& urdf,
                                                         const std::string& baseLinkId,
                                                         const std::string& flangeLinkId)
{
    SerialRobotConfig config;
    config.topology = RobotTopologyType::Serial;
    config.frames.baseLinkId = baseLinkId;
    config.frames.flangeLinkId = flangeLinkId;

    std::vector<Joint> parsedJoints;
    QXmlStreamReader reader(QString::fromStdString(urdf));
    while (!reader.atEnd()) {
        reader.readNext();
        if (!reader.isStartElement()) {
            continue;
        }
        if (reader.name() == QStringLiteral("robot")) {
            config.identity.model = reader.attributes().value("name").toString().toStdString();
        } else if (reader.name() == QStringLiteral("link")) {
            config.links.push_back(Link{reader.attributes().value("name").toString().toStdString()});
        } else if (reader.name() == QStringLiteral("joint")) {
            Joint joint;
            joint.id = reader.attributes().value("name").toString().toStdString();
            joint.type = jointTypeFromName(reader.attributes().value("type").toString());
            while (!(reader.isEndElement() && reader.name() == QStringLiteral("joint")) && !reader.atEnd()) {
                reader.readNext();
                if (!reader.isStartElement()) {
                    continue;
                }
                if (reader.name() == QStringLiteral("parent")) {
                    joint.parentLinkId = reader.attributes().value("link").toString().toStdString();
                } else if (reader.name() == QStringLiteral("child")) {
                    joint.childLinkId = reader.attributes().value("link").toString().toStdString();
                } else if (reader.name() == QStringLiteral("origin")) {
                    const Eigen::Vector3d xyz = parseVec3(reader.attributes().value("xyz").toString(), Eigen::Vector3d::Zero());
                    const Eigen::Vector3d rpy = parseVec3(reader.attributes().value("rpy").toString(), Eigen::Vector3d::Zero());
                    joint.origin = Pose::fromXYZRPY_m_rad(xyz.x(), xyz.y(), xyz.z(), rpy.x(), rpy.y(), rpy.z());
                } else if (reader.name() == QStringLiteral("axis")) {
                    joint.axis = parseVec3(reader.attributes().value("xyz").toString(), Eigen::Vector3d::UnitZ());
                } else if (reader.name() == QStringLiteral("limit")) {
                    JointLimits limits;
                    limits.lower = reader.attributes().value("lower").toDouble();
                    limits.upper = reader.attributes().value("upper").toDouble();
                    joint.limits = limits;
                }
            }
            if (joint.type != JointType::Fixed && !joint.limits.has_value()) {
                joint.limits = JointLimits{-3.141592653589793, 3.141592653589793, std::nullopt, std::nullopt};
            }
            parsedJoints.push_back(joint);
        }
    }
    if (reader.hasError()) {
        return Result<SerialRobotConfig>::failure(KinematicsStatus::InvalidRobotConfig, "URDF XML parse error");
    }

    std::string current = baseLinkId;
    while (current != flangeLinkId) {
        auto next = std::find_if(parsedJoints.begin(), parsedJoints.end(), [&](const Joint& joint) {
            return joint.parentLinkId == current;
        });
        if (next == parsedJoints.end()) {
            return Result<SerialRobotConfig>::failure(KinematicsStatus::InvalidRobotConfig,
                                                      "URDF serial chain is incomplete");
        }
        config.joints.push_back(*next);
        current = next->childLinkId;
    }

    int movable = 0;
    for (const Joint& joint : config.joints) {
        if (joint.type == JointType::Revolute || joint.type == JointType::Prismatic) {
            ++movable;
        }
    }
    config.dof = movable;

    const ModelValidationResult validation = RobotModelValidator::validateSerialRobotConfig(config);
    if (!validation.ok()) {
        return Result<SerialRobotConfig>::failure(validation.status(), validation.issues.front().message);
    }
    return Result<SerialRobotConfig>::success(config);
}

} // namespace RobotKinematics
