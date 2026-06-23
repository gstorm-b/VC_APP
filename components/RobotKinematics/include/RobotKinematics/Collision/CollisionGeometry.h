#pragma once

#include <RobotKinematics/Core/Pose.h>

#include <string>

namespace RobotKinematics {

enum class CollisionShapeType {
    Sphere,
    Capsule
};

struct CollisionSphere {
    double radius_m = 0.0;
};

struct CollisionCapsule {
    double radius_m = 0.0;
    double length_m = 0.0;
};

struct CollisionShape {
    CollisionShapeType type = CollisionShapeType::Sphere;
    CollisionSphere sphere;
    CollisionCapsule capsule;
};

struct CollisionGeometry {
    std::string id;
    std::string linkId;
    CollisionShape shape;
    Pose geometryToLink = Pose::identity();
    double margin_m = 0.0;
    bool enabled = true;
};

struct DisabledCollisionPair {
    std::string geometryA;
    std::string geometryB;
    std::string reason;
};

} // namespace RobotKinematics
