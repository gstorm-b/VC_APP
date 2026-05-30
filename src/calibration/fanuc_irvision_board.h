#ifndef FANUC_IRVISION_BOARD_H
#define FANUC_IRVISION_BOARD_H

#include "calibration_board.h"

namespace calib {

// =====================================================================
// FanucIRvisionBoard - Fanuc iRVision-style dot grid (concrete board)
// =====================================================================
//
// Pattern: R x C grid of dots with 4 visual roles:
//   - Normal dots : small solid black dots filling the grid
//   - Target dots : 8 ring dots (4 corners + 4 edge midpoints), white inner
//   - Centre dot  : 1 larger ring dot at the grid centre, white inner
//   - Coord dots  : 3 larger solid dots near centre encoding +X (two dots
//                   right of centre) and +Y (one dot above centre)
//
// rows and cols MUST be odd; rows >= 5 and cols >= 7.
//
class FanucIRvisionBoard : public CalibrationBoard
{
public:
    struct Params {
        int    rows                  = 11;    // must be odd, >= 5
        int    cols                  = 11;    // must be odd, >= 7
        double dotSpacingMm          = 15.0;
        double marginMm              = 15.0;
        // Diameter of (a) plain solid normal dots, and (b) outer of target rings.
        double normalDotSizeMm       = 4.0;
        // Diameter of (a) outer of the centre ring, and (b) the 3 solid coord dots.
        double coorDotSizeMm         = 8.0;
        // Inner white circle of target rings and the centre ring.
        double innerTargetDotSizeMm  = 1.0;

        bool isValid(std::string* errorOut = nullptr) const;
    };

    // Built-in presets (named after the physical iRVision part dot pitch).
    constexpr static const Params iRvisionPattern5mm  {17, 17,  5.0,    10.0,   2.0,    4,      1.0};
    constexpr static const Params iRvisionPattern11m  {11, 11,  11.5,   10.0,   2.0,    4,      1.0};
    constexpr static const Params iRvisionPattern15mm {11, 11,  15.0,   10.0,   6.0,    11.0,   1.0};
    constexpr static const Params iRvisionPattern22mm {11, 11,  22.5,   10.0,   10.0,   16.0,   1.0};
    constexpr static const Params iRvisionPattern30mm {17, 17,  30.0,   10.0,   2,      4.0,    1.0};

    // Throws std::invalid_argument if params are invalid.
    explicit FanucIRvisionBoard(const Params& params = Params{11, 11, 15.0, 15.0, 4.0, 8.0, 1.0});

    const Params& params() const { return m_params; }

    // --- CalibrationBoard interface ---
    cv::Mat generateImage(double pixelsPerMm = 10.0) const override;
    bool    detect(const cv::Mat& image,
                   std::vector<cv::Point2f>& imagePoints,
                   std::vector<cv::Point2f>* cornerImagePoints = nullptr,
                   cv::Mat* debugOverlay = nullptr) const override;

    int                      totalDots()            const override { return m_params.rows * m_params.cols; }
    std::vector<cv::Point3f> objectPoints()         const override;
    std::vector<cv::Point2f> objectPointsXY()       const override;
    std::vector<cv::Point2f> cornerObjectPointsXY() const override;

    std::string typeName() const override { return "FanucIRvision"; }

    // Read the Params from a JSON document produced by toJson().
    // Throws std::invalid_argument if any parsed Params field is invalid.
    static FanucIRvisionBoard::Params paramsFromJson(const std::string& json);

    // --- iRVision-specific accessors ---
    std::vector<int> targetIndices() const;   // 8 target rings  (row-major idx)
    std::vector<int> centerIndex()   const;   // 1 centre ring
    std::vector<int> coordIndices()  const;   // 3 coord dots [xMid, xTip, yTip]
    std::vector<int> markerIndices() const;   // [centre, xMid, xTip, yTip]

protected:
    void writeJsonFields(cv::FileStorage& fs) const override;

private:
    struct GridCell { int row; int col; };

    std::vector<GridCell> targetCells() const;
    GridCell              centerCell() const;
    std::vector<GridCell> coordCells() const;
    std::vector<GridCell> cornerCells() const;

    enum class DotRole { Normal, Target, Center, Coord };
    DotRole roleOf(int row, int col) const;

    Params m_params;
};

} // namespace calib

#endif // FANUC_IRVISION_BOARD_H
