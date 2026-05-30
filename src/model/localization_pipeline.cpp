#include "model/localization_pipeline.h"

#include "logger/app_logger.h"
#include "matching/image_matcher.h"
#include "matching/match_group.h"
#include "matching/match_pattern.h"

namespace vc::model {

mtc::MatchResult LocalizationPipeline::runMatch(std::shared_ptr<mtc::MatchGroup> group,
                                                const cv::Mat &image) const
{
    mtc::MatchResult emptyResult;
    if (!group) {
        LOG_USER_ERR << "Localization matching failed: match group is null.";
        return emptyResult;
    }

    if (image.empty()) {
        LOG_USER_ERR << "Localization matching failed: source image is empty.";
        return emptyResult;
    }

    mtc::ImageMatcher matcher;
    auto *model = matcher.getModel();
    group->cloneConfigTo(*model);

    for (const auto &patternPtr : group->patterns()) {
        if (!patternPtr)
            continue;

        mtc::MatchPatternConfig cfg = patternPtr->config();
        if (cfg.m_rawImage.empty())
            continue;

        auto r = model->addPattern(cfg);
        if (!r)
            continue;

        auto last = model->lastPatternAccess();
        if (!last)
            continue;

        cv::Mat trainImg = cfg.m_rawImage.clone();
        if (!last->learnPattern(trainImg)) {
            LOG_USER_ERR << "Localization matching failed: learnPattern failed for"
                         << QString::fromStdWString(cfg.m_patternName);
        }
    }

    if (model->isEmpty()) {
        LOG_USER_ERR << "Localization matching failed: matching model is empty.";
        return emptyResult;
    }

    matcher.setImageSource(image.clone());
    matcher.matching(true, -1, false);
    return matcher.match_result;
}

} // namespace vc::model
