#include "calibration_board.h"

#include <opencv2/core/persistence.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <typeinfo>
#include <vector>

namespace calib {

// =====================================================================
// CalibrationBoard base methods
// =====================================================================
cv::Mat CalibrationBoard::pointsToMat(const std::vector<cv::Point2f>& pts)
{
    return cv::Mat(static_cast<int>(pts.size()), 1, CV_32FC2,
                   const_cast<cv::Point2f*>(pts.data())).clone();
}

cv::Mat CalibrationBoard::pointsToMat(const std::vector<cv::Point3f>& pts)
{
    return cv::Mat(static_cast<int>(pts.size()), 1, CV_32FC3,
                   const_cast<cv::Point3f*>(pts.data())).clone();
}

std::string CalibrationBoard::typeName() const
{
    return typeid(*this).name();
}

cv::Mat CalibrationBoard::binarize(const cv::Mat& image, double* usedThreshold) const
{
    cv::Mat gray;
    if (image.channels() == 1) {
        gray = image;
    } else {
        cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
    }

    cv::Mat binary;
    const int t = binarizeThreshold();
    double applied = 0.0;
    if (t < 0) {
        // Auto: Otsu picks the threshold; cv::threshold returns the value used.
        applied = cv::threshold(gray, binary, 0, 255,
                                cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    } else {
        applied = static_cast<double>(t);
        cv::threshold(gray, binary, applied, 255, cv::THRESH_BINARY_INV);
    }
    if (usedThreshold) *usedThreshold = applied;
    return binary;
}

std::string CalibrationBoard::toJson() const
{
    cv::FileStorage fs(".json",
                       cv::FileStorage::WRITE | cv::FileStorage::MEMORY
                                              | cv::FileStorage::FORMAT_JSON);
    if (!fs.isOpened()) return {};
    fs << "type" << typeName();
    writeJsonFields(fs);
    return fs.releaseAndGetString();
}

bool CalibrationBoard::writePrintableImage(const std::string& path, double dpi) const
{
    if (dpi <= 0.0) return false;
    const double pxPerMm = pixelsPerMmFromDpi(dpi);
    const cv::Mat img    = generateImage(pxPerMm);
    return writePrintablePng(path, img, pxPerMm);
}

// =====================================================================
// Printable PNG with embedded pHYs (physical pixel dimensions) chunk
// =====================================================================
namespace {

uint32_t pngCrc32(const uint8_t* data, size_t len)
{
    static uint32_t table[256];
    static bool     ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k)
                c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i)
        c = table[(c ^ data[i]) & 0xFFu] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

void writeU32BE(std::vector<uint8_t>& out, uint32_t v)
{
    out.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    out.push_back(static_cast<uint8_t>((v >>  8) & 0xFF));
    out.push_back(static_cast<uint8_t>( v        & 0xFF));
}

} // namespace

bool writePrintablePng(const std::string& path,
                       const cv::Mat& img,
                       double pxPerMm)
{
    if (img.empty() || pxPerMm <= 0.0) return false;

    std::vector<uchar> encoded;
    if (!cv::imencode(".png", img, encoded)) return false;

    static const uint8_t sig[8] = { 0x89, 'P','N','G', 0x0D, 0x0A, 0x1A, 0x0A };
    if (encoded.size() < 16 ||
        !std::equal(sig, sig + 8, encoded.begin())) return false;

    size_t insertPos = 0;
    size_t pos = 8;
    while (pos + 12 <= encoded.size()) {
        const uint32_t len = (uint32_t(encoded[pos    ]) << 24) |
                             (uint32_t(encoded[pos + 1]) << 16) |
                             (uint32_t(encoded[pos + 2]) <<  8) |
                              uint32_t(encoded[pos + 3]);
        const char* type = reinterpret_cast<const char*>(&encoded[pos + 4]);
        if (std::memcmp(type, "IDAT", 4) == 0) { insertPos = pos; break; }
        pos += 12u + len;
    }
    if (insertPos == 0) return false;

    const uint32_t pxPerMeter = static_cast<uint32_t>(std::round(pxPerMm * 1000.0));
    std::vector<uint8_t> chunk;
    chunk.reserve(21);
    writeU32BE(chunk, 9);
    const size_t typeBegin = chunk.size();
    chunk.push_back('p'); chunk.push_back('H');
    chunk.push_back('Y'); chunk.push_back('s');
    writeU32BE(chunk, pxPerMeter);
    writeU32BE(chunk, pxPerMeter);
    chunk.push_back(0x01);                                // unit = meter
    const uint32_t crc = pngCrc32(chunk.data() + typeBegin, chunk.size() - typeBegin);
    writeU32BE(chunk, crc);

    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(encoded.data()),
            static_cast<std::streamsize>(insertPos));
    f.write(reinterpret_cast<const char*>(chunk.data()),
            static_cast<std::streamsize>(chunk.size()));
    f.write(reinterpret_cast<const char*>(encoded.data() + insertPos),
            static_cast<std::streamsize>(encoded.size() - insertPos));
    return f.good();
}

} // namespace calib
