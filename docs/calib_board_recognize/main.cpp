#include <QCoreApplication>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

#include <array>
#include <iostream>
#include <iomanip>
#include <memory>
#include <random>
#include <stdexcept>

#include "calibration/calibration_board.h"
#include "calibration/calibration_board_factory.h"
#include "calibration/fanuc_irvision_board.h"
#include "calibration/calibrator.h"

namespace {

void printMat(const std::string& name, const cv::Mat& m)
{
    std::cout << name << " (" << m.rows << "x" << m.cols << "):\n";
    std::cout << std::fixed << std::setprecision(6);
    for (int r = 0; r < m.rows; ++r) {
        std::cout << "  [ ";
        for (int c = 0; c < m.cols; ++c) {
            std::cout << std::setw(12) << m.at<double>(r, c) << " ";
        }
        std::cout << "]\n";
    }
}

// Simulate "robot world" by transforming board-mm points with a known affine
// in X,Y plus a (possibly tilted) work plane in Z. In a real setup the (x,y,z)
// values come from probing the corner markers with the robot tool.
//   tiltA/tiltB define a robot-frame work plane z = tiltA * x + tiltB * y + tiltC.
std::vector<cv::Point3f> simulateRobotPoints(const std::vector<cv::Point2f>& boardMm,
                                              double rotDeg,
                                              const cv::Point2f& translation,
                                              double scale,
                                              double noiseStdMm,
                                              double tiltA = 0.0,
                                              double tiltB = 0.0,
                                              double tiltC = 0.0)
{
    const double a = rotDeg * CV_PI / 180.0;
    const float cosA = static_cast<float>(std::cos(a) * scale);
    const float sinA = static_cast<float>(std::sin(a) * scale);

    std::mt19937 rng(123);
    const bool useNoise = (noiseStdMm > 0.0);
    std::normal_distribution<float> noise(0.0f,
                                          useNoise ? static_cast<float>(noiseStdMm) : 1.0f);

    std::vector<cv::Point3f> out;
    out.reserve(boardMm.size());
    for (const auto& p : boardMm) {
        const float nx = useNoise ? noise(rng) : 0.0f;
        const float ny = useNoise ? noise(rng) : 0.0f;
        const float nz = useNoise ? noise(rng) : 0.0f;
        const float xR = cosA * p.x - sinA * p.y + translation.x + nx;
        const float yR = sinA * p.x + cosA * p.y + translation.y + ny;
        const float zR = static_cast<float>(tiltA * xR + tiltB * yR + tiltC) + nz;
        out.emplace_back(xR, yR, zR);
    }
    return out;
}

cv::Mat rotateImage(const cv::Mat& src, double angleDeg)
{
    // Tâm xoay
    cv::Point2f center(src.cols / 2.0f, src.rows / 2.0f);

    // Ma trận xoay
    cv::Mat rot = cv::getRotationMatrix2D(center, angleDeg, 1.0);

    // Tính bounding box mới sau khi xoay
    cv::Rect2f bbox = cv::RotatedRect(cv::Point2f(), src.size(), angleDeg).boundingRect2f();

    // Điều chỉnh ma trận để ảnh không bị lệch
    rot.at<double>(0, 2) += bbox.width / 2.0 - src.cols / 2.0;
    rot.at<double>(1, 2) += bbox.height / 2.0 - src.rows / 2.0;

    cv::Mat dst;
    cv::warpAffine(src, dst, rot, bbox.size());

    return dst;
}

double computeRotationAngle(const std::vector<cv::Point2f>& imgPts,
                            const std::vector<cv::Point2f>& realPts)
{
    // imgPts và realPts đều có 3 điểm: A, B, C
    cv::Point2f A  = imgPts[0];
    cv::Point2f B  = imgPts[1];
    cv::Point2f C  = imgPts[2];

    cv::Point2f A2 = realPts[0];
    cv::Point2f B2 = realPts[1];
    cv::Point2f C2 = realPts[2];

    // Vector trong hệ ảnh
    cv::Point2f u  = { B.x - A.x,  B.y - A.y };
    cv::Point2f v  = { C.x - A.x,  C.y - A.y };

    // Vector trong hệ thực
    cv::Point2f u2 = { B2.x - A2.x, B2.y - A2.y };
    cv::Point2f v2 = { C2.x - A2.x, C2.y - A2.y };

    // Góc giữa u và u'
    double cross1 = u.x * u2.y - u.y * u2.x;
    double dot1   = u.x * u2.x + u.y * u2.y;
    double theta1 = std::atan2(cross1, dot1);

    // Góc giữa v và v'
    double cross2 = v.x * v2.y - v.y * v2.x;
    double dot2   = v.x * v2.x + v.y * v2.y;
    double theta2 = std::atan2(cross2, dot2);

    // Trả về trung bình để giảm nhiễu
    return (theta1 + theta2) / 2.0;
}

void checkAngle() {

    std::vector<cv::Point2f> img = {
        {1335.11, 672.753}, {887.42, 654.378}, {1354.47, 224.355}
    };

    std::vector<cv::Point2f> real = {
        {-130.012, 572.863}, {-121.738, 722.75}, {19.769, 565.18}
    };

    double angle = computeRotationAngle(img, real);

    std::cout << std::endl;
    std::cout << "Rotation angle (rad): " << angle << std::endl;
    std::cout << "Rotation angle (deg): " << angle * 180.0 / M_PI << std::endl;
}

} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    // ---- 1. Configure board through the factory.
    // Generic path: factory returns a CalibrationBoard base pointer so callers
    // can stay board-agnostic. Pass any preset name registered in
    // CalibrationBoardFactory::availablePresets().
    std::unique_ptr<calib::CalibrationBoard> board;
    try {
        board = calib::CalibrationBoardFactory::createFromPreset("iRvision-15mm");
        if (!board) throw std::runtime_error("unknown preset");
    } catch (const std::exception& e) {
        std::cerr << "Failed to construct board: " << e.what() << "\n";
        return 1;
    }

    std::cout << "=== CalibrationBoard (" << board->typeName() << ") ===\n";
    // Type-specific accessors (Params, targetIndices, ...) require down-casting.
    if (const auto* irv = dynamic_cast<const calib::FanucIRvisionBoard*>(board.get())) {
        const auto& bp = irv->params();
        std::cout << "rows=" << bp.rows << "  cols=" << bp.cols
                  << "  spacing=" << bp.dotSpacingMm << "mm\n"
                  << "normal dot / target outer: " << bp.normalDotSizeMm << "mm\n"
                  << "centre outer / coord dot : " << bp.coorDotSizeMm   << "mm\n"
                  << "inner white target       : " << bp.innerTargetDotSizeMm << "mm\n";
    }

    // ---- 2. Generate synthetic image, save to disk ----
    const double pxPerMm = 20.0;
    cv::Mat boardImg = board->generateImage(pxPerMm);
    cv::imwrite("board.png", boardImg);
    std::cout << "Saved board.png  (" << boardImg.cols << "x" << boardImg.rows << " px)\n";

    // cv::Mat boardRotateImg = rotateImage(boardImg, 180.0);
    // boardImg = boardRotateImg.clone();
    cv::Mat img_input = cv::imread("image_15_2.bmp");
    boardImg = img_input.clone();

    // Also save a 300 DPI printable PNG. Embeds a pHYs chunk so the printer
    // can render the board at exact physical size (1:1 mm scale).
    const double printDpi = 600.0;
    if (board->writePrintableImage("board_print_300dpi.png", printDpi)) {
        const double printPxPerMm = calib::CalibrationBoard::pixelsPerMmFromDpi(printDpi);
        std::cout << "Saved board_print_300dpi.png  @ " << printDpi
                  << " DPI  (" << printPxPerMm << " px/mm)\n";
    } else {
        std::cerr << "Failed to write printable PNG.\n";
    }

    // ---- 3. Detect dots + 4 corner markers ----
    std::vector<cv::Point2f> imagePts;
    std::vector<cv::Point2f> cornerImagePts;   // TL, TR, BR, BL
    cv::Mat overlay;
    const bool ok = board->detect(boardImg, imagePts, &cornerImagePts, &overlay);
    std::cout << "Detection: " << (ok ? "OK" : "FAIL")
              << "  detected=" << imagePts.size()
              << " / expected=" << board->totalDots()
              << "  corners=" << cornerImagePts.size() << "\n";
    if (!ok) return 2;
    cv::imwrite("board_detected.png", overlay);
    std::cout << "Saved board_detected.png\n";

    std::cout << "\n--- 4 corners detected (image px) ---\n";
    const std::array<const char*, 4> tag{ "TL", "TR", "BR", "BL" };
    for (int i = 0; i < 4; ++i)
        std::cout << "  " << tag[i] << ": ("
                  << cornerImagePts[i].x << ", " << cornerImagePts[i].y << ")\n";

    // ---- 4. Simulate robot-side measurement of the 4 corners ----
    //         (in production: touch each corner with the robot tool and record XYZ.)
    //         Inject a small tilt so we can demonstrate the plane fit producing Z.
    // const auto cornerBoardMm = board->cornerObjectPointsXY();   // TL, TR, BR, BL in board mm
    // const auto cornerRobotMm = simulateRobotPoints(
    //     cornerBoardMm,
    //     /*rotDeg*/        -90.0,
    //     /*translation mm*/ cv::Point2f(250.0f, -500.0f),
    //     /*scale*/          1.0,
    //     /*noise mm*/       0.05,
    //     /*tiltA*/          0.002,   // z = 0.002*x + 0.001*y + 50  ~ 0.1 mm slope per 50 mm
    //     /*tiltB*/          0.001,
    //     /*tiltC*/          50.0);
    std::vector<cv::Point3f> cornerRobotMm;
    cornerRobotMm.push_back(cv::Point3f(-121.738, 722.750, 107.544));
    cornerRobotMm.push_back(cv::Point3f(27.895, 714.734, 108.544));
    cornerRobotMm.push_back(cv::Point3f(19.769, 565.180, 109.250));
    cornerRobotMm.push_back(cv::Point3f(-130.012, 572.863, 108.451));

    std::cout << "\n--- 4 corners in robot frame (mm, simulated) ---\n";
    for (int i = 0; i < 4; ++i)
        std::cout << "  " << tag[i] << ": ("
                  << cornerRobotMm[i].x << ", "
                  << cornerRobotMm[i].y << ", "
                  << cornerRobotMm[i].z << ")\n";

    // ---- 5. Calibrate using only the 4 corner markers ----
    calib::Calibrator calibrator;
    calibrator.addCorrespondences(cornerImagePts, cornerRobotMm);
    if (!calibrator.calibrate()) {
        std::cerr << "Calibration failed.\n";
        return 3;
    }

    std::cout << "\n=== Calibrator (4-corner) ===\n";
    std::cout << "samples = " << calibrator.sampleCount() << "\n";
    std::cout << "reprojection error: "
              << calibrator.reprojectionErrorPx() << " px,  "
              << calibrator.reprojectionErrorMm() << " mm\n";
    std::cout << "plane fit max Z residual: "
              << calibrator.planeFitMaxErrorMm() << " mm\n";
    const cv::Vec4d plane = calibrator.workPlane();
    std::cout << "work plane (ax+by+cz+d=0): a=" << plane[0]
              << "  b=" << plane[1] << "  c=" << plane[2] << "  d=" << plane[3] << "\n";
    printMat("H (image px -> robot XY mm)", calibrator.homography());

    // ---- 6. Sample conversions ----
    std::cout << "\n--- Sample conversions ---\n";
    const cv::Point2f imgCenter(static_cast<float>(boardImg.cols) * 0.5f,
                                static_cast<float>(boardImg.rows) * 0.5f);
    const cv::Point3f robotAtImgCenter = calibrator.imageToRobot(imgCenter);
    const cv::Point2f backToImg        = calibrator.robotToImage(robotAtImgCenter);
    std::cout << "image center  : (" << imgCenter.x      << ", " << imgCenter.y      << ") px\n"
              << "  -> robot    : (" << robotAtImgCenter.x << ", "
                                     << robotAtImgCenter.y << ", "
                                     << robotAtImgCenter.z << ") mm\n"
              << "  -> back img : (" << backToImg.x      << ", " << backToImg.y      << ") px\n";

    // ---- 6a. Grid centre marker conversion ----
    //   The board has a distinct centre marker (the large ring) at grid index
    //   (rmid, cmid). Take its detected image position, convert to robot mm,
    //   then drive the robot tool to that XYZ and visually confirm the tool
    //   sits on the white centre dot of the board.
    if (const auto* irv = dynamic_cast<const calib::FanucIRvisionBoard*>(board.get())) {
        const int centreIdx        = irv->centerIndex().front();
        const cv::Point2f centreImg = imagePts[centreIdx];
        const cv::Point3f centreRob = calibrator.imageToRobot(centreImg);

        const auto& bp           = irv->params();
        const float boardCx_mm   = static_cast<float>((bp.cols - 1) * 0.5 * bp.dotSpacingMm);
        const float boardCy_mm   = static_cast<float>((bp.rows - 1) * 0.5 * bp.dotSpacingMm);

        const cv::Point2f backImg  = calibrator.robotToImage(centreRob);
        const float pxErr          = static_cast<float>(cv::norm(backImg - centreImg));

        std::cout << "\n--- Grid centre marker (physical centre of board) ---\n";
        std::cout << "  board mm (expected) : (" << boardCx_mm << ", " << boardCy_mm << ", 0)\n";
        std::cout << "  image px (detected) : (" << centreImg.x << ", " << centreImg.y << ")\n";
        std::cout << "  -> robot mm         : (" << centreRob.x << ", "
                                                 << centreRob.y << ", "
                                                 << centreRob.z << ")\n";
        std::cout << "  -> back to image px : (" << backImg.x << ", " << backImg.y
                                                 << ")  round-trip err = " << pxErr << " px\n";
    }

    // ---- 6a-bis. Two random grid-cell spot-checks ----
    //   Pick 2 random grid cells (excluding the 4 corners and the centre, which
    //   were already used or shown). Print board mm, image px, robot XYZ and the
    //   image px round-trip error. Drive the robot tool to the printed XYZ to
    //   verify the calibration on cells that did NOT participate in the fit.
    if (const auto* irv = dynamic_cast<const calib::FanucIRvisionBoard*>(board.get())) {
        const auto& bp       = irv->params();
        const int   total    = irv->totalDots();
        const int   centreIx = irv->centerIndex().front();
        const auto  corners  = irv->cornerObjectPointsXY();  // mm; just used to exclude indices
        const int rmax = bp.rows - 1, cmax = bp.cols - 1;
        const std::array<int, 4> cornerIdx{
            0 * bp.cols + 0,
            0 * bp.cols + cmax,
            rmax * bp.cols + cmax,
            rmax * bp.cols + 0
        };
        auto isReserved = [&](int idx) {
            if (idx == centreIx) return true;
            for (int ci : cornerIdx) if (idx == ci) return true;
            return false;
        };

        std::mt19937 rng(0xC0FFEE);   // deterministic for reproducibility
        std::uniform_int_distribution<int> dist(0, total - 1);

        std::cout << "\n--- Random grid-cell spot-checks ---\n";
        int picked = 0;
        while (picked < 2) {
            const int idx = dist(rng);
            if (isReserved(idx)) continue;
            const int row = idx / bp.cols;
            const int col = idx % bp.cols;

            const cv::Point2f imgPx = imagePts[idx];
            const cv::Point3f robMm = calibrator.imageToRobot(imgPx);
            const cv::Point2f back  = calibrator.robotToImage(robMm);
            const float pxErr       = static_cast<float>(cv::norm(back - imgPx));
            const float bxMm        = static_cast<float>(col * bp.dotSpacingMm);
            const float byMm        = static_cast<float>(row * bp.dotSpacingMm);

            std::cout << "  cell (row=" << row << ", col=" << col << ")\n"
                      << "    board mm (expected) : (" << bxMm << ", " << byMm << ", 0)\n"
                      << "    image px (detected) : (" << imgPx.x << ", " << imgPx.y << ")\n"
                      << "    -> robot mm         : (" << robMm.x << ", "
                                                       << robMm.y << ", "
                                                       << robMm.z << ")\n"
                      << "    -> back image px    : (" << back.x << ", " << back.y
                                                       << ")  err = " << pxErr << " px\n";
            ++picked;
        }
    }

    // ---- 6b. Angle round-trip ----
    std::cout << "\n--- Angle conversions ---\n";
    const double anglesImgDeg[] = { 0.0, 30.0, 90.0, 180.0, -45.0 };
    for (double a : anglesImgDeg) {
        const double rob = calibrator.rotateImageToRobot(a);
        const double back = calibrator.rotateRobotToImage(rob);
        std::cout << "  image " << a << " deg  ->  robot R " << rob
                  << " deg  ->  back image " << back << " deg\n";
    }

    // ---- 8. Save / reload ----
    const std::string yamlPath = "calibration.yml";
    if (calibrator.save(yamlPath)) {
        std::cout << "\nSaved " << yamlPath << "\n";
    }
    calib::Calibrator reloaded;
    if (reloaded.load(yamlPath)) {
        const cv::Point3f r2 = reloaded.imageToRobot(imgCenter);
        std::cout << "Reloaded H, image-center -> robot = ("
                  << r2.x << ", " << r2.y << ", " << r2.z << ") mm\n";
    }

    // ----- 9. Check rotation angle ----
    checkAngle();

    return 0;
}
