/**
 *
 * PatternPoint contains coordinates, deriavtive and magnitude of each pattern's point, which extract from edge
 * PattenrLayer contains learned data from pattern iamge
 *
 */

#ifndef MATCH_PATTERN_LAYER_H
#define MATCH_PATTERN_LAYER_H

#include <vector>
#include <opencv2/core.hpp>

namespace mtc {

struct PatternPoint {
    cv::Point Coordinates;
    cv::Point2f Derivative;
    float Magnitude;
};

class PatternLayer {
public:
    PatternLayer() :
        pGx(nullptr),
        pGy(nullptr),
        pMag(nullptr){

    }

    ~PatternLayer() {
        this->freeMemory();
    }

    void allocateMemory(size_t num) {
        freeMemory();
        pGx = (float*)_mm_malloc(num * sizeof(float), 32);
        pGy = (float*)_mm_malloc(num * sizeof(float), 32);
        pMag = (float*)_mm_malloc(num * sizeof(float), 32);
    }

    void freeMemory() {
        if (pGx) {
            _mm_free(pGx);
            pGx = nullptr;
        }
        if (pGy) {
            _mm_free(pGy);
            pGy = nullptr;
        }

        if (pMag) {
            _mm_free(pMag);
            pMag = nullptr;
        }
    }

    // copy constructor
    PatternLayer(const PatternLayer& other) {
        image = other.image.clone();
        patternPoints = other.patternPoints;
        contours = other.contours;
        hierarchies = other.hierarchies;
        Magnitude = other.Magnitude.clone();
        angle = other.angle.clone();

        size_t numOfPts = patternPoints.size();
        allocateMemory(numOfPts);
        std::memcpy(pGx, other.pGx, numOfPts * sizeof(float));
        std::memcpy(pGy, other.pGy, numOfPts * sizeof(float));
        std::memcpy(pMag, other.pMag, numOfPts * sizeof(float));
    }

    // copy assignment
    PatternLayer& operator=(const PatternLayer& other) {
        if (this != &other) {
            image = other.image.clone();
            patternPoints = other.patternPoints;
            contours = other.contours;
            hierarchies = other.hierarchies;
            Magnitude = other.Magnitude.clone();
            angle = other.angle.clone();

            size_t numOfPts = patternPoints.size();
            allocateMemory(numOfPts);
            std::memcpy(pGx, other.pGx, numOfPts * sizeof(float));
            std::memcpy(pGy, other.pGy, numOfPts * sizeof(float));
            std::memcpy(pMag, other.pMag, numOfPts * sizeof(float));
        }
        return *this;
    }

    // Move constructor
    PatternLayer(PatternLayer&& other) noexcept
        : image(other.image),
        patternPoints(other.patternPoints),
        hierarchies(other.hierarchies),
        Magnitude(other.Magnitude),
        angle(other.angle),
        pGx(other.pGx),
        pGy(other.pGy),
        pMag(other.pMag) {

        other.pGx = nullptr;
        other.pGy = nullptr;
        other.pMag = nullptr;
    }

    // Move assignment
    PatternLayer& operator=(PatternLayer&& other) noexcept {
        if (this != &other) {
            image = other.image;
            patternPoints = other.patternPoints;
            contours = other.contours;
            hierarchies = other.hierarchies;
            Magnitude = other.Magnitude;
            angle = other.angle;

            freeMemory();
            pGx = other.pGx;
            pGy = other.pGy;
            pMag = other.pMag;
            other.pGx = nullptr;
            other.pGy = nullptr;
            other.pMag = nullptr;
        }
        return *this;
    }

public:
    cv::Mat image;
    std::vector<PatternPoint> patternPoints;
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchies;
    cv::Mat Magnitude;
    cv::Mat angle;
    float *pGx{nullptr};
    float *pGy{nullptr};
    float *pMag{nullptr};
};

}

#endif // MATCH_PATTERN_LAYER_H
