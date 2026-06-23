#pragma once

#include <RobotKinematics/Collision/CollisionProfile.h>
#include <RobotKinematics/Collision/StlMeshLoader.h>
#include <RobotKinematics/Core/Result.h>

#include <array>
#include <cstddef>
#include <string>

namespace RobotKinematics {

struct StlMeshStatistics {
    std::size_t triangleCount = 0;
    std::array<double, 3> minimumBounds_m = {0.0, 0.0, 0.0};
    std::array<double, 3> maximumBounds_m = {0.0, 0.0, 0.0};
    std::array<double, 3> centroid_m = {0.0, 0.0, 0.0};
    std::array<double, 3> axisLengths_m = {0.0, 0.0, 0.0};
};

struct StlPrimitiveProposalRequest {
    std::string profileId = "draft_collision_profile";
    std::string robotModel = "draft_robot";
    std::string geometryId = "draft_geometry";
    std::string linkId = "draft_link";
    double margin_m = 0.0;
    bool enabled = true;
};

struct StlPrimitiveProposal {
    StlFileFormat format = StlFileFormat::Ascii;
    StlMeshStatistics statistics;
    CollisionGeometry sphere;
    CollisionGeometry capsule;
    std::string sphereDraftJson;
    std::string capsuleDraftJson;
    std::string reviewNote;
};

// Authoring helper only: parses simple ASCII/binary STL, computes conservative
// primitive proposals, and emits draft JSON snippets that must be reviewed by a
// human before becoming runtime collision profiles.
class StlPrimitiveAuthoringHelper
{
public:
    static Result<StlPrimitiveProposal> proposeFromFile(const std::string& path,
                                                        const StlPrimitiveProposalRequest& request = {});
};

} // namespace RobotKinematics
