#ifndef MATCH_BOX_GRIPPER_H
#define MATCH_BOX_GRIPPER_H

#include <opencv2/opencv.hpp>

namespace mtc {

class MatchBoxGripper {
public:
    MatchBoxGripper() {}

public:
    cv::Size2f m_boxSize;
    double m_boxDistance;
};

}

#endif // MATCH_BOX_GRIPPER_H
