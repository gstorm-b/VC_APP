#ifndef UTILS_BLOCK_MAX_H
#define UTILS_BLOCK_MAX_H

#include <vector>
#include <opencv2/core.hpp>

namespace mtc {

class MatchBlock {
public:
    MatchBlock();
    MatchBlock(cv::Rect rect, double val_max, cv::Point ptMaxLoc);
    ~MatchBlock();

public:
    cv::Rect _rect;
    double _maxValue;
    cv::Point _pointMaxLoc;
};

class BlockMax {
public:
    BlockMax();
    BlockMax(cv::Mat matSrc, cv::Size sizePattern);
    ~BlockMax();

    void UpdateMax(cv::Rect rectIgnore);
    void GetMaxValueLoc(double &dMax, cv::Point& ptMaxLoc);

private:
    std::vector<MatchBlock> _vecBlocks;
    cv::Mat _matSrc;
};

}

#endif // UTILS_BLOCK_MAX_H
