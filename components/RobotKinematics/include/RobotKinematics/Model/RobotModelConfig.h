#pragma once

#include <RobotKinematics/Core/Pose.h>

#include <Eigen/Core>

#include <map>
#include <optional>
#include <string>
#include <vector>

namespace RobotKinematics {

enum class RobotTopologyType {
    Serial,
    Scara,
    Parallel,
    Custom
};

enum class JointType {
    Revolute,
    Prismatic,
    Fixed
};

struct RobotIdentity {
    std::string vendor;
    std::string model;
    std::string name;
    std::string revision;
};

struct Link {
    std::string id;
};

struct JointLimits {
    double lower = 0.0;
    double upper = 0.0;
    std::optional<double> velocity;
    std::optional<double> acceleration;
};

struct Joint {
    std::string id;
    JointType type = JointType::Revolute;
    std::string parentLinkId;
    std::string childLinkId;
    Pose origin = Pose::identity();
    Eigen::Vector3d axis = Eigen::Vector3d::UnitZ();
    std::optional<JointLimits> limits;
    double home = 0.0;
    std::vector<std::string> aliases;
};

struct UserFrame {
    std::string id;
    std::string parentLinkId;
    Pose transform = Pose::identity();
};

struct FrameConfig {
    std::string baseLinkId;
    std::string flangeLinkId;
    std::vector<UserFrame> userFrames;
};

struct Tool {
    std::string id;
    std::string name;
    Pose flangeToTcp = Pose::identity();
};

struct PostureLabelAxis {
    std::string negative;
    std::string positive;
};

struct PostureMetadata {
    std::string resolver;
    std::map<std::string, PostureLabelAxis> labels;
};

struct SolverMetadata {
    std::string defaultSolver;
    std::map<std::string, double> numericParameters;
};

struct SourceReference {
    std::string type;
    std::string title;
    std::string reference;
    std::vector<std::string> appliesTo;
};

struct SerialRobotConfig {
    RobotIdentity identity;
    RobotTopologyType topology = RobotTopologyType::Serial;
    int dof = 0;
    std::vector<Link> links;
    std::vector<Joint> joints;
    FrameConfig frames;
    std::vector<Tool> tools;
    std::string defaultToolId;
    PostureMetadata posture;
    SolverMetadata solver;
    std::vector<SourceReference> sources;
    std::map<std::string, std::string> metadata;
};

} // namespace RobotKinematics
