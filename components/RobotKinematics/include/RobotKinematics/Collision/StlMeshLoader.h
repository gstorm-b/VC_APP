#pragma once

#include <RobotKinematics/Collision/TriangleMesh.h>
#include <RobotKinematics/Core/Result.h>

#include <string>

namespace RobotKinematics {

struct StlMeshLoadOptions {
    double scaleToMeters = 1.0;
    bool rejectDegenerateTriangles = true;
};

class StlMeshLoader
{
public:
    static Result<TriangleMesh> loadFile(const std::string& path,
                                         const StlMeshLoadOptions& options = {});
};

} // namespace RobotKinematics
