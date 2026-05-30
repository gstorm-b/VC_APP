#ifndef LOCALIZATION_PIPELINE_H
#define LOCALIZATION_PIPELINE_H

#include <memory>

#include <opencv2/core/mat.hpp>

#include "matching/image_matcher.h"

namespace mtc {
class MatchGroup;
}

namespace vc::model {

class LocalizationPipeline {
public:
    mtc::MatchResult runMatch(std::shared_ptr<mtc::MatchGroup> group,
                              const cv::Mat &image) const;
};

} // namespace vc::model

#endif // LOCALIZATION_PIPELINE_H
