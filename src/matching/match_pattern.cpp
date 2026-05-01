#include "match_pattern.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include "vision_utils.h"

using namespace std;
using namespace cv;

namespace mtc {

MatchPattern::MatchPattern() {}

MatchPattern::MatchPattern(const MatchPatternConfig& config, MatchGroup* parent)
    : m_config(config), m_parentGroup(parent) {}

// ── Learn ─────────────────────────────────────────────────────────────────────

bool MatchPattern::learnPattern(std::wstring path) {
    m_hasPatternLearned = false;
    this->setRawImage(path);
    if (m_config.m_rawImage.empty())
        return false;
    return learnPattern();
}

bool MatchPattern::learnPattern(cv::Mat& imgPattern) {
    this->setRawImage(imgPattern);
    return this->learnPattern();
}

bool MatchPattern::learnPattern() {
    m_hasPatternLearned = false;
    m_image = m_config.m_rawImage.clone();
    if (m_image.empty())
        return false;

    const EdgeMatchConfig* ecfg = m_config.edgeConfig();
    if (!ecfg)
        return false;  // only EdgeBased is implemented

    clearPatternData();
    this->findTopLayer();
    cv::buildPyramid(m_image, m_pyramids, m_topLayer);
    m_borderColor = mean(m_image).val[0] < 128 ? 255 : 0;

    const int pyr_size = static_cast<int>(m_pyramids.size());
    m_patterns.resize(pyr_size);

    std::vector<std::vector<cv::Point>> top_layer_contours;

    for (int index = 0; index < pyr_size; ++index) {
        PatternLayer pattern;
        pattern.patternPoints.clear();
        pattern.contours.clear();
        pattern.hierarchies.clear();

        Mat imgGray = m_pyramids.at(index).clone();
        GaussianBlur(imgGray, imgGray,
                     cv::Size(ecfg->blurWidth, ecfg->blurHeight), 0);
        Canny(imgGray, pattern.image,
              ecfg->threshLower, ecfg->threshUpper, ecfg->kernelSize);

        // find contours on canny output
        findContours(pattern.image, pattern.contours, pattern.hierarchies,
                     cv::RETR_EXTERNAL, cv::CHAIN_APPROX_NONE);

        Mat gx, gy, magnitude, angle;
        Sobel(imgGray, gx, CV_32F, 1, 0, 3);
        Sobel(imgGray, gy, CV_32F, 0, 1, 3);
        cartToPolar(gx, gy, magnitude, angle, true);

        pattern.angle     = angle;
        pattern.Magnitude = magnitude;

        if (index == 0)
            m_maskPatternPoint = cv::Mat(imgGray.size(), CV_8UC1, cv::Scalar(0));

        for (int ci = 0; ci < static_cast<int>(pattern.contours.size()); ++ci) {
            std::vector<cv::Point> resampled =
                vsu::tNeighborSimplify(pattern.contours[ci], ecfg->tSamples);

            for (int pi = 0; pi < static_cast<int>(resampled.size()); ++pi) {
                PatternPoint pt;
                pt.Coordinates = resampled[pi];
                pt.Derivative  = Point2f(gx.at<float>(pt.Coordinates),
                                         gy.at<float>(pt.Coordinates));
                pt.Magnitude   = magnitude.at<float>(pt.Coordinates);
                pattern.patternPoints.push_back(pt);
            }

            if (index == 0)
                top_layer_contours.push_back(resampled);
        }

        pattern.freeMemory();
        const std::size_t numPts = pattern.patternPoints.size();
        pattern.allocateMemory(numPts);
        for (int count = 0; count < static_cast<int>(numPts); ++count) {
            PatternPoint* tp = &pattern.patternPoints[count];
            pattern.pGx[count]  = tp->Derivative.x;
            pattern.pGy[count]  = tp->Derivative.y;
            pattern.pMag[count] = tp->Magnitude;
        }

        pattern.image  = m_pyramids.at(index).clone();
        m_patterns[index] = pattern;
    }

    if (!m_maskPatternPoint.empty())
        cv::polylines(m_maskPatternPoint, top_layer_contours, true, cv::Scalar(255), 1);

    // ── PCA contour selection (used for area-based pre-filter) ────────────

    int binary_threshold_style = cv::THRESH_OTSU;
    if (ecfg->invertBinaryThreshold)
        binary_threshold_style |= cv::THRESH_BINARY_INV;

    Mat threshImage;
    vector<vector<Point>> pcaContours;
    vector<Vec4i> pcaHierarchy;
    Mat imgGray = m_image.clone();
    threshold(imgGray, threshImage, 0, 255, binary_threshold_style);
    findContours(threshImage, pcaContours, pcaHierarchy,
                 cv::RETR_TREE, cv::CHAIN_APPROX_NONE);

    const int limitArea = static_cast<int>(imgGray.cols * imgGray.rows * 0.98);
    int maxArea = 0;
    for (int ci = 0; ci < static_cast<int>(pcaContours.size()); ++ci) {
        int area = static_cast<int>(contourArea(pcaContours[ci]));
        if (area >= limitArea) continue;
        if (area > maxArea) {
            maxArea = area;
            m_contoursSelectIndex    = ci;
            m_contoursSelectArea     = maxArea;
            m_contoursSelectRect     = cv::minAreaRect(pcaContours[ci]);
            m_contoursSelectRectArea = static_cast<int>(
                m_contoursSelectRect.size.height * m_contoursSelectRect.size.width);
        }
    }
    m_maskImage = threshImage.clone();

    m_hasPatternLearned = true;
    return true;
}

// ── Config ────────────────────────────────────────────────────────────────────

void MatchPattern::setConfig(MatchPatternConfig& cfg) {
    m_config = cfg;
}

MatchPatternConfig MatchPattern::patternConfig() const {
    return m_config;
}

const MatchPatternConfig* MatchPattern::patternConfigPtr() const {
    return &m_config;
}

// ── Image API ─────────────────────────────────────────────────────────────────

void MatchPattern::setRawImage(std::wstring path) {
    if (path.empty()) return;
    cv::Mat raw = vsu::imread_wstring(path);
    this->setRawImage(raw);
}

void MatchPattern::setRawImage(cv::Mat& imgPattern) {
    m_hasPatternLearned = false;
    if (imgPattern.empty()) return;
    if (imgPattern.channels() == 3)
        cv::cvtColor(imgPattern, m_config.m_rawImage, cv::COLOR_BGR2GRAY);
    else if (imgPattern.channels() == 4)
        cv::cvtColor(imgPattern, m_config.m_rawImage, cv::COLOR_BGRA2GRAY);
    else
        m_config.m_rawImage = imgPattern.clone();
}

cv::Mat MatchPattern::getRawImage() const {
    return m_config.m_rawImage.clone();
}

cv::MatSize MatchPattern::getRawImageSize() {
    return m_config.m_rawImage.size;
}

cv::Mat MatchPattern::getImageWithPickPosition() {
    if (m_image.empty()) return cv::Mat();
    cv::Mat out;
    if (m_image.channels() == 1)
        cv::cvtColor(m_image, out, cv::COLOR_GRAY2BGR);
    else
        out = m_image.clone();
    vsu::drawAxes2Img(out, m_config.m_pickPosition, m_config.m_angle,
                      false, 20, 0.4, DEFAULT_COLOR_BLUE, DEFAULT_COLOR_RED);
    return out;
}

cv::Mat MatchPattern::getImageWithCannyThreshold() {
    if (m_image.empty()) return cv::Mat();

    const EdgeMatchConfig* ecfg = m_config.edgeConfig();
    const int kernelSize = ecfg ? ecfg->kernelSize : 3;

    double cannyRange     = 50;
    double CANNY_RANGE_MAX = 100;
    double cannyLow, cannyHigh;

    cv::Mat cannyImg = m_image.clone();
    GaussianBlur(cannyImg, cannyImg, cv::Size(5, 5), 0);
    double med   = vsu::median(cannyImg);
    double sigma = (0.1 / CANNY_RANGE_MAX) * cannyRange;
    cannyLow  = std::max(0.0,  (1.0 - sigma) * med);
    cannyHigh = std::min(255.0, (1.0 + sigma) * med);
    Canny(cannyImg, cannyImg, cannyLow, cannyHigh, kernelSize, true);
    return cannyImg;
}

// ── Private helpers ───────────────────────────────────────────────────────────

void MatchPattern::clearPatternData() {
    m_patterns.clear();
}

void MatchPattern::findTopLayer() {
    const EdgeMatchConfig* ecfg = m_config.edgeConfig();
    const int minLen = ecfg ? ecfg->minReduceLength : 32;
    m_minReduceArea = minLen * minLen;
    m_topLayer = 0;
    int area = m_image.cols * m_image.rows;
    while (area > m_minReduceArea) {
        area /= 4;
        ++m_topLayer;
    }
}

// ── Friend (MatchGroup) API ───────────────────────────────────────────────────

ManagerResult MatchPattern::setConfig(const MatchPatternConfig& cfg) {
    m_config = cfg;
    return ManagerResult::success();
}

ManagerResult MatchPattern::setName(const std::wstring& name) {
    m_config.m_patternName = name;
    return ManagerResult::success();
}

ManagerResult MatchPattern::setNumber(int number) {
    m_config.m_patternIndex = number;
    return ManagerResult::success();
}

void MatchPattern::setImage(const cv::Mat& image) {
    m_config.m_rawImage = image.clone();
    m_image = image.clone();
}

} // namespace mtc
