#ifndef MATCH_PARAMS_H
#define MATCH_PARAMS_H

#include <opencv2/core.hpp>

namespace mtc {

class MatchParams {
public:
    MatchParams();
    MatchParams(cv::Point2f ptMinMax, double score, double angle);
    ~MatchParams();

public:
    cv::Point2d _point;
    double _matchScore;
    double _matchAngle;

    // Mat _matRotatedSrc;
    cv::Rect _rectRoi;
    double _angleStart;
    double _angleEnd;
    cv::RotatedRect _rectR;
    cv::Rect _rectBounding;
    bool _delete;

    // for subpixel
    double _vecResult[3][3];
    int _maxScoreIndex;
    bool _posOnBorder;
    cv::Point2d _pointSubPixel;
    double _newAngle;
};

}

#endif // MATCH_PARAMS_H
