#ifndef IMAGE_MATCHER_H
#define IMAGE_MATCHER_H

#include "utils_block_max.h"
#include "match_params.h"
#include "match_object.h"
#include "match_group.h"
#include "edge_match_config.h"
#include "robot_picking_checker.h"

#define SOURCE_THERSHOLD_TOLERANCE  (double)0.3

namespace mtc {

struct MatchResult {
    std::vector<MatchedObject> Objects;
    cv::Point2f cropOffsetPoint;
    double      ExecutionTime{0.0};
    cv::Mat     Image;
    int         imageCols{0};
    int         imageRows{0};
    bool        isFoundMatchObject{false};
    int         totalPossiblePicking{0};
    bool        isAreaLessThanLimits{false};
};

// ---------------------------------------------------------------------------
// ImageMatcher — edge-based template matching engine.
//
// Algorithm config lives in the group's MatchGroupConfig::typeConfig
// (EdgeMatchConfig), shared by every pattern in the group.  Runtime search
// parameters (max_pos_num, max_overlap) are kept here as they govern the
// search over the whole image, not a single pattern.
// ---------------------------------------------------------------------------
class ImageMatcher {
public:
    ImageMatcher();

    MatchGroup* getPatternGroup();
    MatchGroup* getModel() { return &m_model_src; }

    int  getTemplateSourceSize();
    void setImageSource(std::string path);
    void setImageSource(cv::Mat img);
    void setMatchingROI(cv::Point tl, cv::Point br);
    void setMatchingConditionROI(cv::Point tl, cv::Point br);

    void matching(cv::Mat& image, bool boundingBoxChecking, int objectsNum, bool usingRoi, bool usingConditionRoi);
    void matching(bool boundingBoxChecking, int objectsNum, bool usingRoi,  bool usingConditionRoi);

    void setMatchSourceImage(cv::Mat& img);
    bool MatchEdge(cv::Mat& imgEdge, MatchPattern* pattern,
                   std::vector<MatchedObject>& matchObjs);

    // Sort matched objects in place by pick priority, then by score or angle.
    //
    // Primary key — pick priority (best first):
    //   1. no collision AND inside the condition ROI
    //   2. collision   AND inside the condition ROI
    //   3. the rest (outside the condition ROI)
    //
    // Secondary key — applied within each priority group:
    //   - highest matched_Score first (default), or
    //   - if the group config's m_sortByAngle is set, smallest angular error of
    //     point_angle vs m_sortConditionAngle first (score is ignored).
    void sortMatchedObjects(std::vector<MatchedObject>& objects);

    // Inject the robot-pickability checker (dependency inversion; non-owning).
    // Pass nullptr to disable. The owner builds the concrete adapter from the
    // calibration + robot config and keeps it alive while this matcher is used.
    void setRobotPickingChecker(const IRobotPickingChecker* checker) {
        m_pickingChecker = checker;
    }

    // Whether the robot can actually pick this matched object. Sequential steps:
    //   1. convert the object's image position/angle to a world pick pose,
    //   2. solveAll IK to confirm the pose is reachable,
    //   3. (only if the robot config enabled it) simplified-mesh self-collision.
    // Returns true when no checker is injected (the check is advisory: an
    // unconfigured robot does not gate matching).
    //
    // Frame note: the image coordinates handed to the checker are
    // point_Center + match_result.cropOffsetPoint, i.e. the matcher's own input
    // frame. If the host pre-crops the image before matching, it must inject a
    // checker whose calibration is consistent with that frame (or set the
    // matching ROI here instead of pre-cropping).
    bool robotPossiblePickingCheck(const MatchedObject& obj) const;

public:
    int    max_pos_num = 70;
    double max_overlap = 0.2;
    MatchResult match_result;

private:
    struct PossibleObject {
        int           conIndex;
        cv::Rect      conBoundingRect;
        cv::RotatedRect conMinRectArea;
        std::vector<int> modelCheckList;
    };

    // EdgeBased matching steps — cfg provides per-pattern algorithm settings
    bool MatchEdgePattern(cv::Mat& matSrc, MatchPattern* model,
                          cv::Mat& matResult, int layer, double minScore,
                          const EdgeMatchConfig& cfg);
    bool MatchEdgePattern_SIMD(cv::Mat& matSrc, MatchPattern* model,
                               cv::Mat& matResult, int layer, double minScore);

    cv::Size  GetBestRotationSize(cv::Size sizeSrc, cv::Size sizeDst, double rAngle);
    cv::Point2f ptRotatePt2f(cv::Point2f ptInput, cv::Point2f ptOrg, double angle);
    cv::Point GetNextMaxLoc(cv::Mat& matResult, cv::Point ptMaxLoc,
                            cv::Size sizeTemplate, double& maxValue, double maxOverlap);
    cv::Point GetNextMaxLoc(cv::Mat& matResult, cv::Point ptMaxLoc,
                            cv::Size sizeTemplate, double& maxValue,
                            double maxOverlap, BlockMax& blockMax);
    void GetRotatedROI(cv::Mat& matSrc, cv::Size size, cv::Point2f ptLT,
                       double dAngle, cv::Mat& matROI);
    bool SubPixEsimation(std::vector<MatchParams>* vec, double* dNewX,
                         double* dNewY, double* dNewAngle,
                         double dAngleStep, int iMaxScoreIndex);
    void FilterWithScore(std::vector<MatchParams>* vec, double dScore);
    void FilterWithRotatedRect(std::vector<MatchParams>* vec, int iMethod,
                               double dMaxOverLap,
                               std::vector<int>* del_indexes = nullptr);
    void SortPtWithCenter(std::vector<cv::Point2f>& vecSort);
    void DrawMatchResult(cv::Mat& drawImage,
                         std::vector<MatchedObject>& matchedResults);
    bool pointInBox(const cv::Point2f& tl, const cv::Point2f& br, const cv::Point2f& pt);

private:
    cv::Mat      m_img_source;
    MatchGroup   m_model_src;
    cv::Point2f  ROI_tl;
    cv::Point2f  ROI_br;
    cv::Point2f  Condition_ROI_tl;
    cv::Point2f  Condition_ROI_br;

    std::vector<MatchParams> m_final_overlap_result;

    // Non-owning robot-pickability checker (nullptr => check disabled).
    const IRobotPickingChecker* m_pickingChecker{nullptr};

    const double lowerThreshRatio = 1.0 - SOURCE_THERSHOLD_TOLERANCE;
    const double upperThreshRatio = 4.0 + SOURCE_THERSHOLD_TOLERANCE;
};

} // namespace mtc
#endif // IMAGE_MATCHER_H
