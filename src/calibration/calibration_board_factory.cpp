#include "calibration_board_factory.h"

#include <opencv2/core/persistence.hpp>

#include <stdexcept>

namespace calib {

std::unique_ptr<CalibrationBoard>
CalibrationBoardFactory::create(CalibrationBoardType type)
{
    switch (type) {
    case CalibrationBoardType::FanucIRvision:
        return createFanucIRvision();
    }
    return nullptr;
}

std::unique_ptr<CalibrationBoard>
CalibrationBoardFactory::createFanucIRvision(const FanucIRvisionBoard::Params& params)
{
    return std::make_unique<FanucIRvisionBoard>(params);
}

std::unique_ptr<CalibrationBoard>
CalibrationBoardFactory::createFromPreset(const std::string& presetName)
{
    if (presetName == "iRvision-5mm")
        return createFanucIRvision(FanucIRvisionBoard::iRvisionPattern5mm);
    if (presetName == "iRvision-11.5mm")
        return createFanucIRvision(FanucIRvisionBoard::iRvisionPattern11m);
    if (presetName == "iRvision-15mm")
        return createFanucIRvision(FanucIRvisionBoard::iRvisionPattern15mm);
    if (presetName == "iRvision-22.5mm")
        return createFanucIRvision(FanucIRvisionBoard::iRvisionPattern22mm);
    if (presetName == "iRvision-30mm")
        return createFanucIRvision(FanucIRvisionBoard::iRvisionPattern30mm);
    return nullptr;
}

std::unique_ptr<CalibrationBoard>
CalibrationBoardFactory::createFromJson(const std::string& json)
{
    cv::FileStorage fs(json,
                       cv::FileStorage::READ | cv::FileStorage::MEMORY
                                             | cv::FileStorage::FORMAT_JSON);
    if (!fs.isOpened()) return nullptr;

    std::string type;
    fs["type"] >> type;
    fs.release();

    if (type == "FanucIRvision") {
        try {
            return std::make_unique<FanucIRvisionBoard>(
                FanucIRvisionBoard::paramsFromJson(json));
        } catch (const std::invalid_argument&) {
            return nullptr;
        }
    }
    return nullptr;
}

std::vector<std::string> CalibrationBoardFactory::availablePresets()
{
    return {
        "iRvision-5mm",
        "iRvision-11.5mm",
        "iRvision-15mm",
        "iRvision-22.5mm",
        "iRvision-30mm"
    };
}

} // namespace calib
