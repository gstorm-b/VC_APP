#include <RobotKinematics/Collision/StlMeshLoader.h>

#include <QByteArray>
#include <QFile>
#include <QRegularExpression>
#include <QString>
#include <QStringList>

#include <Eigen/Geometry>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

namespace RobotKinematics {

namespace {
Result<TriangleMesh> invalid(const std::string& message)
{
    return Result<TriangleMesh>::failure(KinematicsStatus::InvalidRequest, message);
}

std::uint32_t readLeUInt32(const QByteArray& bytes, int offset)
{
    std::uint32_t value = 0;
    std::memcpy(&value, bytes.constData() + offset, sizeof(value));
    return value;
}

float readLeFloat(const QByteArray& bytes, int offset)
{
    float value = 0.0f;
    std::memcpy(&value, bytes.constData() + offset, sizeof(value));
    return value;
}

bool isFinite(const Eigen::Vector3d& vertex)
{
    return std::isfinite(vertex.x()) && std::isfinite(vertex.y()) && std::isfinite(vertex.z());
}

bool isDegenerateTriangle(const Eigen::Vector3d& a,
                          const Eigen::Vector3d& b,
                          const Eigen::Vector3d& c)
{
    return ((b - a).cross(c - a)).norm() <= 1e-12;
}

bool appendTriangle(TriangleMesh& mesh,
                    const Eigen::Vector3d& a,
                    const Eigen::Vector3d& b,
                    const Eigen::Vector3d& c,
                    const StlMeshLoadOptions& options,
                    std::string& error)
{
    if (!isFinite(a) || !isFinite(b) || !isFinite(c)) {
        error = "STL mesh contains non-finite vertex values";
        return false;
    }
    if (isDegenerateTriangle(a, b, c)) {
        if (options.rejectDegenerateTriangles) {
            error = "STL mesh contains a degenerate triangle";
            return false;
        }
        return true;
    }

    const std::size_t baseIndex = mesh.vertices_m.size();
    mesh.vertices_m.push_back(a);
    mesh.vertices_m.push_back(b);
    mesh.vertices_m.push_back(c);
    mesh.faces.push_back(TriangleFace{baseIndex, baseIndex + 1, baseIndex + 2});
    return true;
}

void finalizeStatistics(TriangleMesh& mesh, const double scaleToMeters)
{
    Eigen::Vector3d minimum = Eigen::Vector3d::Constant(std::numeric_limits<double>::max());
    Eigen::Vector3d maximum = Eigen::Vector3d::Constant(std::numeric_limits<double>::lowest());

    for (const Eigen::Vector3d& vertex : mesh.vertices_m) {
        minimum = minimum.cwiseMin(vertex);
        maximum = maximum.cwiseMax(vertex);
    }

    mesh.statistics.vertexCount = mesh.vertices_m.size();
    mesh.statistics.triangleCount = mesh.faces.size();
    mesh.statistics.minimumBounds_m = {minimum.x(), minimum.y(), minimum.z()};
    mesh.statistics.maximumBounds_m = {maximum.x(), maximum.y(), maximum.z()};
    mesh.statistics.scaleToMeters = scaleToMeters;
}

Result<TriangleMesh> parseBinary(const QByteArray& bytes, const StlMeshLoadOptions& options)
{
    if (bytes.size() < 84) {
        return invalid("STL payload is too small to be a binary STL");
    }

    const std::uint32_t triangleCount = readLeUInt32(bytes, 80);
    const std::uint64_t expectedSize = 84ull + static_cast<std::uint64_t>(triangleCount) * 50ull;
    if (expectedSize != static_cast<std::uint64_t>(bytes.size())) {
        return invalid("binary STL size does not match triangle count");
    }

    TriangleMesh mesh;
    mesh.sourceFormat = StlFileFormat::Binary;
    mesh.vertices_m.reserve(static_cast<std::size_t>(triangleCount) * 3);
    mesh.faces.reserve(static_cast<std::size_t>(triangleCount));

    std::string error;
    int offset = 84;
    for (std::uint32_t triangleIndex = 0; triangleIndex < triangleCount; ++triangleIndex) {
        offset += 12;

        const Eigen::Vector3d a(readLeFloat(bytes, offset) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 4) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 8) * options.scaleToMeters);
        offset += 12;
        const Eigen::Vector3d b(readLeFloat(bytes, offset) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 4) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 8) * options.scaleToMeters);
        offset += 12;
        const Eigen::Vector3d c(readLeFloat(bytes, offset) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 4) * options.scaleToMeters,
                                readLeFloat(bytes, offset + 8) * options.scaleToMeters);
        offset += 12;

        if (!appendTriangle(mesh, a, b, c, options, error)) {
            return invalid(error);
        }

        offset += 2;
    }

    if (mesh.faces.empty()) {
        return invalid("binary STL contains no triangles");
    }

    finalizeStatistics(mesh, options.scaleToMeters);
    return Result<TriangleMesh>::success(mesh);
}

Result<TriangleMesh> parseAscii(const QByteArray& bytes, const StlMeshLoadOptions& options)
{
    const QString text = QString::fromUtf8(bytes);
    const QStringList lines = text.split(QRegularExpression("[\r\n]+"), Qt::SkipEmptyParts);

    std::vector<Eigen::Vector3d> vertices;
    vertices.reserve(static_cast<std::size_t>(lines.size()));

    for (const QString& rawLine : lines) {
        const QString line = rawLine.trimmed();
        if (!line.startsWith(QStringLiteral("vertex "))) {
            continue;
        }

        const QStringList parts = line.split(QRegularExpression("\\s+"), Qt::SkipEmptyParts);
        if (parts.size() != 4) {
            return invalid("ASCII STL vertex line must contain three numeric coordinates");
        }

        bool okX = false;
        bool okY = false;
        bool okZ = false;
        const double x = parts[1].toDouble(&okX);
        const double y = parts[2].toDouble(&okY);
        const double z = parts[3].toDouble(&okZ);
        if (!okX || !okY || !okZ) {
            return invalid("ASCII STL contains a non-numeric vertex coordinate");
        }

        vertices.emplace_back(x * options.scaleToMeters,
                              y * options.scaleToMeters,
                              z * options.scaleToMeters);
    }

    if (vertices.empty() || (vertices.size() % 3) != 0) {
        return invalid("ASCII STL does not contain a whole number of triangles");
    }

    TriangleMesh mesh;
    mesh.sourceFormat = StlFileFormat::Ascii;
    mesh.vertices_m.reserve(vertices.size());
    mesh.faces.reserve(vertices.size() / 3);

    std::string error;
    for (std::size_t i = 0; i < vertices.size(); i += 3) {
        if (!appendTriangle(mesh, vertices[i], vertices[i + 1], vertices[i + 2], options, error)) {
            return invalid(error);
        }
    }

    if (mesh.faces.empty()) {
        return invalid("ASCII STL contains no triangles after filtering degenerate faces");
    }

    finalizeStatistics(mesh, options.scaleToMeters);
    return Result<TriangleMesh>::success(mesh);
}
}

Result<TriangleMesh> StlMeshLoader::loadFile(const std::string& path,
                                             const StlMeshLoadOptions& options)
{
    if (options.scaleToMeters <= 0.0) {
        return invalid("STL mesh scaleToMeters must be positive");
    }

    QFile file(QString::fromStdString(path));
    if (!file.open(QIODevice::ReadOnly)) {
        return invalid("STL mesh file could not be opened");
    }

    const QByteArray bytes = file.readAll();

    Result<TriangleMesh> result = parseBinary(bytes, options);
    if (result.ok()) {
        return result;
    }

    const Result<TriangleMesh> asciiResult = parseAscii(bytes, options);
    if (asciiResult.ok()) {
        return asciiResult;
    }

    return asciiResult;
}

} // namespace RobotKinematics
