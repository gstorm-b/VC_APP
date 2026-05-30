#ifndef CALIBRATOR_H
#define CALIBRATOR_H

#include <opencv2/core.hpp>
#include <opencv2/core/persistence.hpp>
#include <string>
#include <vector>

namespace calib {

// =====================================================================
// Calibrator
// =====================================================================
//
// Holds a planar 2D <-> 3D calibration between the camera image and the robot
// world. Internally it fits:
//   * a 3x3 homography H mapping image (px) <-> robot XY (mm) from N >= 4
//     point correspondences, AND
//   * a 3D plane (in robot mm) through the supplied 3D robot points so that
//     imageToRobot() can report a Z that follows a slightly-tilted work surface.
//
// Angle conversions go through H empirically (mapped vectors at the dataset
// centroid), with image angle measured counter-clockwise from +X in the
// "Y-points-down" image frame and robot R measured counter-clockwise from +X
// around +Z (standard right-hand-rule top-down). Degrees by default, radians
// available via an optional flag.
//
class Calibrator
{
public:
    Calibrator();

    void clear();

    // Each robot point carries its measured Z, so a (possibly tilted) work
    // plane can be fitted during calibrate().
    void addCorrespondences(const std::vector<cv::Point2f>& imagePoints,
                            const std::vector<cv::Point3f>& robotPointsMm);

    bool calibrate(double ransacReprojThreshold = 3.0);

    bool isCalibrated() const { return m_calibrated; }

    // image (px) -> robot (mm). Returned Z lies on the fitted work plane.
    cv::Point3f imageToRobot(const cv::Point2f& imagePx) const;

    // robot (mm) -> image (px). Input Z is ignored; only XY is back-projected.
    cv::Point2f robotToImage(const cv::Point3f& robotMm) const;

    std::vector<cv::Point3f> imageToRobot(const std::vector<cv::Point2f>& imagePts) const;
    std::vector<cv::Point2f> robotToImage(const std::vector<cv::Point3f>& robotPts) const;

    // Angle conversion. radians=false (default) interprets/returns degrees.
    // image angle convention: CCW from +X, taken in the Y-down image frame.
    // robot angle convention: CCW from +X around +Z (top-down view).
    double rotateImageToRobot(double angleImg, bool radians = false) const;
    double rotateRobotToImage(double angleRob, bool radians = false) const;

    // Translate with Rotated Z axis
    cv::Point3f translateWithZAxis(cv::Point3f &A, cv::Point3f &offset, double theta, double isRad = true) const;

    // --- Diagnostics ---
    double reprojectionErrorPx()   const { return m_reprojErrorPx; }
    double reprojectionErrorMm()   const { return m_reprojErrorMm; }   // XY residual on robot side
    double planeFitMaxErrorMm()    const { return m_planeFitMaxErr; } // largest robot-Z residual

    cv::Mat   homography()        const { return m_H.clone(); }       // image px -> robot mm (XY)
    cv::Mat   homographyInverse() const { return m_Hinv.clone(); }
    cv::Vec4d workPlane()         const { return m_plane; }           // a*x + b*y + c*z + d = 0

    size_t sampleCount() const { return m_imagePts.size(); }

    bool save(const std::string& filePath) const;
    bool load(const std::string& filePath);

    // In-memory JSON serialisation of the full instance state (homography,
    // work plane, centroids, raw correspondences, diagnostics). fromJson()
    // restores the calibrator to an "already-calibrated" state without
    // re-running the homography fit.
    std::string toJson()                       const;
    bool        fromJson(const std::string& json);

private:
    std::vector<cv::Point2f> m_imagePts;
    std::vector<cv::Point3f> m_robotPts;

    cv::Mat   m_H;
    cv::Mat   m_Hinv;
    cv::Vec4d m_plane{ 0.0, 0.0, 1.0, 0.0 };   // default: z = 0 plane

    // Cached centroid (anchor) for angle conversions.
    cv::Point2f m_imgCentroid{};
    cv::Point2f m_robCentroid{};

    bool   m_calibrated      = false;
    double m_reprojErrorPx   = -1.0;
    double m_reprojErrorMm   = -1.0;
    double m_planeFitMaxErr  = -1.0;

    // Shared parser used by both load(file) and fromJson(string).
    bool readFromStorage(cv::FileStorage& fs);
};

} // namespace calib

#endif // CALIBRATOR_H
