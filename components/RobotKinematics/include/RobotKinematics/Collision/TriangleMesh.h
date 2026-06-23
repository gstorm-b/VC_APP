#pragma once

#include <Eigen/Core>

#include <array>
#include <cstddef>
#include <vector>

namespace RobotKinematics {

enum class StlFileFormat {
    Ascii,
    Binary
};

struct TriangleFace {
    std::size_t a = 0;
    std::size_t b = 0;
    std::size_t c = 0;
};

struct TriangleMeshStatistics {
    std::size_t vertexCount = 0;
    std::size_t triangleCount = 0;
    std::array<double, 3> minimumBounds_m = {0.0, 0.0, 0.0};
    std::array<double, 3> maximumBounds_m = {0.0, 0.0, 0.0};
    double scaleToMeters = 1.0;
};

struct TriangleMesh {
    StlFileFormat sourceFormat = StlFileFormat::Ascii;
    std::vector<Eigen::Vector3d> vertices_m;
    std::vector<TriangleFace> faces;
    TriangleMeshStatistics statistics;
};

} // namespace RobotKinematics
