#include "calibrator.h"

#include <opencv2/calib3d.hpp>
#include <opencv2/core/persistence.hpp>

#include <algorithm>
#include <cmath>

namespace calib {

namespace {

// Fit a plane ax + by + cz + d = 0 through N >= 3 points by SVD on the
// centred coordinates. Returns false if N < 3 or the matrix is degenerate.
bool fitPlane(const std::vector<cv::Point3f>& pts, cv::Vec4d& plane)
{
    if (pts.size() < 3) return false;

    cv::Vec3d centroid(0.0, 0.0, 0.0);
    for (const auto& p : pts) {
        centroid[0] += p.x;
        centroid[1] += p.y;
        centroid[2] += p.z;
    }
    centroid *= (1.0 / static_cast<double>(pts.size()));

    cv::Mat A(static_cast<int>(pts.size()), 3, CV_64F);
    for (int i = 0; i < static_cast<int>(pts.size()); ++i) {
        A.at<double>(i, 0) = pts[i].x - centroid[0];
        A.at<double>(i, 1) = pts[i].y - centroid[1];
        A.at<double>(i, 2) = pts[i].z - centroid[2];
    }

    cv::Mat w, u, vt;
    cv::SVD::compute(A, w, u, vt, cv::SVD::MODIFY_A);
    if (vt.rows < 3) return false;

    const cv::Vec3d normal(vt.at<double>(2, 0),
                           vt.at<double>(2, 1),
                           vt.at<double>(2, 2));
    const double nLen = std::sqrt(normal[0]*normal[0]
                                + normal[1]*normal[1]
                                + normal[2]*normal[2]);
    if (nLen < 1e-9) return false;

    plane[0] = normal[0] / nLen;
    plane[1] = normal[1] / nLen;
    plane[2] = normal[2] / nLen;
    plane[3] = -(plane[0] * centroid[0]
               + plane[1] * centroid[1]
               + plane[2] * centroid[2]);

    // Sign convention: keep the normal pointing toward +Z when possible so that
    // c > 0 (a non-vertical work plane has a well-defined z = f(x, y)).
    if (plane[2] < 0.0) {
        plane[0] = -plane[0];
        plane[1] = -plane[1];
        plane[2] = -plane[2];
        plane[3] = -plane[3];
    }
    return true;
}

double planeZ(const cv::Vec4d& plane, double x, double y)
{
    const double a = plane[0], b = plane[1], c = plane[2], d = plane[3];
    if (std::abs(c) < 1e-12) return 0.0;   // vertical plane: z undefined
    return -(a * x + b * y + d) / c;
}

constexpr double kDeg2Rad = CV_PI / 180.0;
constexpr double kRad2Deg = 180.0 / CV_PI;

} // namespace

// =====================================================================
// Construction / state
// =====================================================================
Calibrator::Calibrator() = default;

void Calibrator::clear()
{
    m_imagePts.clear();
    m_robotPts.clear();
    m_H.release();
    m_Hinv.release();
    m_plane          = cv::Vec4d(0.0, 0.0, 1.0, 0.0);
    m_imgCentroid    = {};
    m_robCentroid    = {};
    m_calibrated     = false;
    m_reprojErrorPx  = -1.0;
    m_reprojErrorMm  = -1.0;
    m_planeFitMaxErr = -1.0;
}

void Calibrator::addCorrespondences(const std::vector<cv::Point2f>& imagePoints,
                                    const std::vector<cv::Point3f>& robotPointsMm)
{
    if (imagePoints.size() != robotPointsMm.size()) return;
    m_imagePts.insert(m_imagePts.end(), imagePoints.begin(),  imagePoints.end());
    m_robotPts.insert(m_robotPts.end(), robotPointsMm.begin(), robotPointsMm.end());
    m_calibrated = false;
}

// =====================================================================
// Calibration
// =====================================================================
bool Calibrator::calibrate(double ransacReprojThreshold)
{
    m_calibrated = false;
    m_H.release();
    m_Hinv.release();

    if (m_imagePts.size() < 4) return false;
    if (m_imagePts.size() != m_robotPts.size()) return false;

    // ---- 1. Homography from image px to robot XY ----
    std::vector<cv::Point2f> robotXY;
    robotXY.reserve(m_robotPts.size());
    for (const auto& p : m_robotPts) robotXY.emplace_back(p.x, p.y);

    cv::Mat H = cv::findHomography(m_imagePts, robotXY, cv::RANSAC, ransacReprojThreshold);
    if (H.empty()) return false;

    cv::Mat Hinv;
    try { Hinv = H.inv(); } catch (const cv::Exception&) { return false; }
    if (Hinv.empty()) return false;

    m_H    = H;
    m_Hinv = Hinv;

    // ---- 2. Work plane (3D) fit through robot 3D points ----
    if (!fitPlane(m_robotPts, m_plane)) return false;

    // ---- 3. Diagnostics ----
    std::vector<cv::Point2f> projRobotXY, projImage;
    cv::perspectiveTransform(m_imagePts, projRobotXY, m_H);
    cv::perspectiveTransform(robotXY,    projImage,   m_Hinv);

    double sumMm = 0.0, sumPx = 0.0, maxZErr = 0.0;
    for (size_t i = 0; i < m_imagePts.size(); ++i) {
        sumMm += cv::norm(projRobotXY[i] - robotXY[i]);
        sumPx += cv::norm(projImage[i]   - m_imagePts[i]);
        const double zPredicted = planeZ(m_plane, m_robotPts[i].x, m_robotPts[i].y);
        maxZErr = std::max(maxZErr, std::abs(zPredicted - m_robotPts[i].z));
    }
    const double n = static_cast<double>(m_imagePts.size());
    m_reprojErrorMm  = sumMm  / n;
    m_reprojErrorPx  = sumPx  / n;
    m_planeFitMaxErr = maxZErr;

    // ---- 4. Cache centroids used as anchors for angle conversions ----
    cv::Point2f imgC(0.f, 0.f), robC(0.f, 0.f);
    for (const auto& p : m_imagePts) imgC += p;
    for (const auto& p : m_robotPts) robC += cv::Point2f(p.x, p.y);
    m_imgCentroid = imgC * (1.0f / static_cast<float>(n));
    m_robCentroid = robC * (1.0f / static_cast<float>(n));

    m_calibrated = true;
    return true;
}

// =====================================================================
// image <-> robot
// =====================================================================
cv::Point3f Calibrator::imageToRobot(const cv::Point2f& imagePx) const
{
    if (!m_calibrated) return { 0.f, 0.f, 0.f };
    std::vector<cv::Point2f> in{ imagePx }, out;
    cv::perspectiveTransform(in, out, m_H);
    const cv::Point2f& xy = out.front();
    const double z = planeZ(m_plane, xy.x, xy.y);
    return cv::Point3f(xy.x, xy.y, static_cast<float>(z));
}

cv::Point2f Calibrator::robotToImage(const cv::Point3f& robotMm) const
{
    if (!m_calibrated) return { 0.f, 0.f };
    std::vector<cv::Point2f> in{ cv::Point2f(robotMm.x, robotMm.y) }, out;
    cv::perspectiveTransform(in, out, m_Hinv);
    return out.front();
}

std::vector<cv::Point3f> Calibrator::imageToRobot(const std::vector<cv::Point2f>& imagePts) const
{
    std::vector<cv::Point3f> out;
    if (!m_calibrated || imagePts.empty()) return out;
    std::vector<cv::Point2f> xy;
    cv::perspectiveTransform(imagePts, xy, m_H);
    out.reserve(xy.size());
    for (const auto& p : xy) {
        const double z = planeZ(m_plane, p.x, p.y);
        out.emplace_back(p.x, p.y, static_cast<float>(z));
    }
    return out;
}

std::vector<cv::Point2f> Calibrator::robotToImage(const std::vector<cv::Point3f>& robotPts) const
{
    std::vector<cv::Point2f> out;
    if (!m_calibrated || robotPts.empty()) return out;
    std::vector<cv::Point2f> xy;
    xy.reserve(robotPts.size());
    for (const auto& p : robotPts) xy.emplace_back(p.x, p.y);
    cv::perspectiveTransform(xy, out, m_Hinv);
    return out;
}

// =====================================================================
// Angle conversion
// =====================================================================
// Empirical method: map the dataset-centroid anchor + a unit-direction step
// through H (or H^-1). Anchor at the centroid keeps the angle estimate well-
// conditioned even when the homography contains mild perspective.
//
// Image convention:  CCW from +X with image Y pointing down
//                    -> direction(theta) = (cos theta, -sin theta)
// Robot convention:  CCW from +X around +Z (right-hand rule, top-down)
//                    -> direction(R)     = (cos R,      sin R)
//
double Calibrator::rotateImageToRobot(double angleImg, bool radians) const
{
    if (!m_calibrated) return 0.0;
    const double theta = radians ? angleImg : angleImg * kDeg2Rad;

    const float step = 100.0f;   // arbitrary length; only angle of result matters
    const cv::Point2f imgDir(static_cast<float>( std::cos(theta)),
                             static_cast<float>(-std::sin(theta)));
    const cv::Point2f imgP0 = m_imgCentroid;
    const cv::Point2f imgP1 = m_imgCentroid + imgDir * step;

    std::vector<cv::Point2f> in{ imgP0, imgP1 }, out;
    cv::perspectiveTransform(in, out, m_H);
    const cv::Point2f robDir = out[1] - out[0];
    const double angleRob = std::atan2(robDir.y, robDir.x);
    return radians ? angleRob : angleRob * kRad2Deg;
}

double Calibrator::rotateRobotToImage(double angleRob, bool radians) const
{
    if (!m_calibrated) return 0.0;
    const double theta = radians ? angleRob : angleRob * kDeg2Rad;

    const float step = 100.0f;
    const cv::Point2f robDir(static_cast<float>(std::cos(theta)),
                             static_cast<float>(std::sin(theta)));
    const cv::Point2f robP0 = m_robCentroid;
    const cv::Point2f robP1 = m_robCentroid + robDir * step;

    std::vector<cv::Point2f> in{ robP0, robP1 }, out;
    cv::perspectiveTransform(in, out, m_Hinv);
    const cv::Point2f imgDir = out[1] - out[0];
    const double angleImg = std::atan2(-imgDir.y, imgDir.x);
    return radians ? angleImg : angleImg * kRad2Deg;
}

// =====================================================================
// Persistence
// =====================================================================
bool Calibrator::save(const std::string& filePath) const
{
    if (!m_calibrated) return false;
    cv::FileStorage fs(filePath, cv::FileStorage::WRITE);
    if (!fs.isOpened()) return false;
    fs << "homography_image_to_robot" << m_H;
    fs << "work_plane_abcd"           << cv::Mat(m_plane, false);
    fs << "image_centroid"            << cv::Mat(cv::Vec2f(m_imgCentroid.x, m_imgCentroid.y), false);
    fs << "robot_centroid"            << cv::Mat(cv::Vec2f(m_robCentroid.x, m_robCentroid.y), false);
    fs << "reprojection_error_px"    << m_reprojErrorPx;
    fs << "reprojection_error_mm"    << m_reprojErrorMm;
    fs << "plane_fit_max_error_mm"   << m_planeFitMaxErr;
    fs << "sample_count"             << static_cast<int>(m_imagePts.size());
    fs.release();
    return true;
}

bool Calibrator::load(const std::string& filePath)
{
    cv::FileStorage fs(filePath, cv::FileStorage::READ);
    if (!fs.isOpened()) return false;

    cv::Mat H;
    fs["homography_image_to_robot"] >> H;
    if (H.empty() || H.rows != 3 || H.cols != 3) return false;

    cv::Mat Hinv;
    try { Hinv = H.inv(); } catch (const cv::Exception&) { return false; }
    if (Hinv.empty()) return false;

    cv::Mat planeMat;
    fs["work_plane_abcd"] >> planeMat;
    if (planeMat.empty() || planeMat.total() < 4) return false;
    m_plane = cv::Vec4d(planeMat.at<double>(0),
                        planeMat.at<double>(1),
                        planeMat.at<double>(2),
                        planeMat.at<double>(3));

    cv::Mat imgC, robC;
    fs["image_centroid"] >> imgC;
    fs["robot_centroid"] >> robC;
    if (!imgC.empty() && imgC.total() >= 2)
        m_imgCentroid = cv::Point2f(imgC.at<float>(0), imgC.at<float>(1));
    if (!robC.empty() && robC.total() >= 2)
        m_robCentroid = cv::Point2f(robC.at<float>(0), robC.at<float>(1));

    m_H    = H;
    m_Hinv = Hinv;
    fs["reprojection_error_px"]   >> m_reprojErrorPx;
    fs["reprojection_error_mm"]   >> m_reprojErrorMm;
    fs["plane_fit_max_error_mm"]  >> m_planeFitMaxErr;
    m_calibrated = true;
    fs.release();
    return true;
}

} // namespace calib
