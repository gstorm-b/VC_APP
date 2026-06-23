#ifndef LOCALIZATION_PIPELINE_H
#define LOCALIZATION_PIPELINE_H

#include <memory>

#include <opencv2/core/mat.hpp>

#include "matching/image_matcher.h"
#include "model/camera_workspace.h"

namespace mtc {
class MatchGroup;
}

namespace vc::model {

class LocalizationPipeline {
public:
    // Max objects the runtime cycle searches for (commission searches all: -1).
    static constexpr int kRuntimeMaxObjects = 2;

    // Load the group's learned model into the matcher (clone config +
    // learnPattern). Returns false if the resulting model is empty. Expensive —
    // the runtime path calls this only when the active group changes.
    bool loadModel(mtc::ImageMatcher &matcher,
                   const std::shared_ptr<mtc::MatchGroup> &group) const;

    // Run matching on a matcher whose model is already loaded: applies the
    // workspace ROI, injects the (optional) robot-pickability checker, sets the
    // image, and matches. maxObjects < 0 means "find all".
    mtc::MatchResult runMatchOn(mtc::ImageMatcher &matcher,
                                const CameraWorkspace &workspace,
                                const cv::Mat &image,
                                const mtc::IRobotPickingChecker *pickingChecker,
                                int maxObjects) const;

    // Commission/test matching: builds a throwaway matcher per call (not the hot
    // path). Loads the model and searches all objects.
    mtc::MatchResult runMatchCommision(std::shared_ptr<mtc::MatchGroup> group,
                              const CameraWorkspace &workspace,
                              const cv::Mat &image,
                              const mtc::IRobotPickingChecker *pickingChecker = nullptr) const;
};

} // namespace vc::model

#endif // LOCALIZATION_PIPELINE_H
