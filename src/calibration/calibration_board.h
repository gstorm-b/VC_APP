#ifndef CALIBRATION_BOARD_H
#define CALIBRATION_BOARD_H

#include <opencv2/core.hpp>
#include <string>
#include <vector>

namespace calib {

// Write a PNG file with an embedded pHYs chunk so the printer / viewer can
// render the image at the correct physical size (1:1 mm scale).
//   pxPerMm: the rendering resolution of `img` (e.g. 300/25.4 for 300 DPI).
// Returns true on success.
bool writePrintablePng(const std::string& path,
                       const cv::Mat& img,
                       double pxPerMm);

// =====================================================================
// CalibrationBoard - abstract base class
// =====================================================================
//
// Every concrete board type (Fanuc iRVision, Halcon, ChArUco, ...) implements
// this interface so that downstream code (Calibrator, GUI, automation scripts)
// can stay board-agnostic.
//
// Concrete subclasses live in their own headers, e.g. FanucIRvisionBoard in
// "fanuc_irvision_board.h". Use CalibrationBoardFactory (in
// "calibration_board_factory.h") to construct instances polymorphically.
//
class CalibrationBoard
{
public:
    virtual ~CalibrationBoard() = default;

    // --- Synthesis ---
    // Render the board into a single-channel image with the given pixel scale.
    virtual cv::Mat generateImage(double pixelsPerMm = 10.0) const = 0;

    // Render the board at `dpi` DPI and write it as a PNG with an embedded
    // pHYs chunk so that printer drivers reproduce the exact physical size.
    // Default 300 DPI is fine for laser printers; use 600 DPI for finer ink.
    bool writePrintableImage(const std::string& path, double dpi = 300.0) const;

    // --- Detection ---
    // imagePoints       : all detected dot centres in row-major (objectPoints) order.
    // cornerImagePoints : optional, the 4 corner dots used for homography fit,
    //                     ordered [TL, TR, BR, BL] in board coordinates.
    // debugOverlay      : optional, BGR image with overlay annotations.
    virtual bool detect(const cv::Mat& image,
                        std::vector<cv::Point2f>& imagePoints,
                        std::vector<cv::Point2f>* cornerImagePoints = nullptr,
                        cv::Mat* debugOverlay = nullptr) const = 0;

    // --- Geometry (board coordinates in mm, z = 0 on a planar board) ---
    virtual int                      totalDots()             const = 0;
    virtual std::vector<cv::Point3f> objectPoints()          const = 0;
    virtual std::vector<cv::Point2f> objectPointsXY()        const = 0;
    virtual std::vector<cv::Point2f> cornerObjectPointsXY()  const = 0;

    // Human-readable identifier of the concrete board type, e.g.
    // "FanucIRvision", "HalconChessboard", ... Defaults to the C++ typeid name
    // but subclasses are encouraged to return a stable string.
    virtual std::string typeName() const;

    // --- JSON serialisation ---
    // toJson() is NVI: it writes the type identifier and delegates the
    // subclass-specific fields to writeJsonFields(). To rebuild a board
    // instance from JSON, call CalibrationBoardFactory::createFromJson().
    std::string toJson() const;

    // --- Static helpers ---
    static cv::Mat pointsToMat(const std::vector<cv::Point2f>& pts);
    static cv::Mat pointsToMat(const std::vector<cv::Point3f>& pts);
    static double  pixelsPerMmFromDpi(double dpi)     { return dpi / 25.4; }
    static double  dpiFromPixelsPerMm(double pxPerMm) { return pxPerMm * 25.4; }

protected:
    // Subclass-specific JSON fields. Implementations write their Params under
    // top-level keys; the "type" key is already emitted by toJson().
    virtual void writeJsonFields(cv::FileStorage& fs) const = 0;
};

} // namespace calib

#endif // CALIBRATION_BOARD_H
