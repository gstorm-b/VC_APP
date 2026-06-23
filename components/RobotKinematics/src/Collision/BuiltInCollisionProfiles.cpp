#include <RobotKinematics/Collision/BuiltInCollisionProfiles.h>

#include <RobotKinematics/Core/Pose.h>

namespace RobotKinematics::CollisionProfiles {

namespace {
CollisionGeometry sphere(const std::string& id,
                         const std::string& linkId,
                         const Pose& geometryToLink,
                         double radius_m,
                         double margin_m = 0.0)
{
    CollisionGeometry geometry;
    geometry.id = id;
    geometry.linkId = linkId;
    geometry.shape.type = CollisionShapeType::Sphere;
    geometry.shape.sphere.radius_m = radius_m;
    geometry.geometryToLink = geometryToLink;
    geometry.margin_m = margin_m;
    return geometry;
}

CollisionGeometry capsule(const std::string& id,
                          const std::string& linkId,
                          const Pose& geometryToLink,
                          double radius_m,
                          double length_m,
                          double margin_m = 0.0)
{
    CollisionGeometry geometry;
    geometry.id = id;
    geometry.linkId = linkId;
    geometry.shape.type = CollisionShapeType::Capsule;
    geometry.shape.capsule.radius_m = radius_m;
    geometry.shape.capsule.length_m = length_m;
    geometry.geometryToLink = geometryToLink;
    geometry.margin_m = margin_m;
    return geometry;
}

DisabledCollisionPair disabled(const std::string& a, const std::string& b, const std::string& reason)
{
    return DisabledCollisionPair{a, b, reason};
}
}

CollisionProfile virtual6DofTestArm()
{
    CollisionProfile profile;
    profile.id = "virtual_6dof_test_arm_conservative_primitives";
    profile.robotModel = "Virtual6DofTestArm";
    profile.geometries = {
        sphere("base_body", "base_link", Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.12, 0.0, 0.0, 0.0), 0.17, 0.01),
        sphere("shoulder_body", "link_1", Pose::fromXYZRPY_m_rad(0.08, 0.0, 0.10, 0.0, 0.0, 0.0), 0.08, 0.005),
        capsule("upper_arm_capsule", "link_2",
                Pose::fromXYZRPY_m_rad(0.175, 0.0, 0.0, 0.0, 1.5707963267948966, 0.0),
                0.07, 0.35, 0.005),
        capsule("forearm_capsule", "link_3",
                Pose::fromXYZRPY_m_rad(0.125, 0.0, 0.0, 0.0, 1.5707963267948966, 0.0),
                0.06, 0.25, 0.005),
        sphere("wrist_body", "link_4", Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.09, 0.0, 0.0, 0.0), 0.08, 0.004),
        sphere("wrist_pitch_body", "link_5", Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.05, 0.0, 0.0, 0.0), 0.06, 0.004),
        sphere("flange_body", "flange", Pose::identity(), 0.05, 0.003),
    };
    profile.disabledPairs = {
        disabled("base_body", "shoulder_body", "adjacent_joint_contact"),
        disabled("shoulder_body", "upper_arm_capsule", "adjacent_joint_contact"),
        disabled("upper_arm_capsule", "forearm_capsule", "adjacent_joint_contact"),
        disabled("forearm_capsule", "wrist_body", "adjacent_joint_contact"),
        disabled("wrist_body", "wrist_pitch_body", "adjacent_joint_contact"),
        disabled("wrist_pitch_body", "flange_body", "adjacent_joint_contact"),
    };
    profile.sources = {
        SourceReference{
            "project_fixture",
            "Virtual 6DOF collision approximation",
            "docs/planning/collision_detection_plan.md",
            {"collision_geometry"},
        },
    };
    profile.metadata["approximation"] = "conservative_primitives";
    profile.metadata["safety_rating"] = "not_safety_rated";
    profile.metadata["review_notes"] = "project_owned_virtual_fixture";
    return profile;
}

CollisionProfile nachiMZ04D()
{
    CollisionProfile profile;
    profile.id = "nachi_mz04d_conservative_primitives";
    profile.robotModel = "MZ04D";
    profile.geometries = {
        sphere("base_body", "base_link", Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.17, 0.0, 0.0, 0.0), 0.10, 0.01),
        capsule("shoulder_column", "link_1",
                Pose::fromXYZRPY_m_rad(0.0, 0.0, 0.17, 0.0, 0.0, 0.0),
                0.08, 0.34, 0.005),
        capsule("upper_arm_capsule", "link_2",
                Pose::fromXYZRPY_m_rad(0.16, 0.0, 0.0, 0.0, 1.5707963267948966, 0.0),
                0.045, 0.20, 0.005),
        sphere("elbow_body", "link_3", Pose::fromXYZRPY_m_rad(0.025, -0.12, 0.0, 0.0, 0.0, 0.0), 0.08, 0.005),
        sphere("forearm_body", "link_4", Pose::fromXYZRPY_m_rad(0.0, 0.0, -0.03, 0.0, 0.0, 0.0), 0.03, 0.004),
        sphere("wrist_pitch_body", "link_5", Pose::identity(), 0.05, 0.004),
        sphere("flange_body", "flange", Pose::identity(), 0.025, 0.003),
    };
    profile.disabledPairs = {
        disabled("base_body", "shoulder_column", "adjacent_joint_contact"),
        disabled("shoulder_column", "upper_arm_capsule", "adjacent_joint_contact"),
        disabled("upper_arm_capsule", "elbow_body", "adjacent_joint_contact"),
        disabled("elbow_body", "forearm_body", "adjacent_joint_contact"),
        disabled("forearm_body", "wrist_pitch_body", "adjacent_joint_contact"),
        disabled("wrist_pitch_body", "flange_body", "adjacent_joint_contact"),
    };
    profile.sources = {
        SourceReference{
            "project_estimate",
            "Conservative primitive approximation from Nachi visual CAD assets",
            "presets/Nachi/MZ04",
            {"collision_geometry"},
        },
    };
    profile.metadata["approximation"] = "conservative_primitives";
    profile.metadata["safety_rating"] = "not_safety_rated";
    profile.metadata["review_notes"] = "visual_cad_based_estimate";
    return profile;
}

} // namespace RobotKinematics::CollisionProfiles
