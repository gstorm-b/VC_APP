#ifndef CALIBRATION_BOARD_FACTORY_H
#define CALIBRATION_BOARD_FACTORY_H

#include "calibration_board.h"
#include "fanuc_irvision_board.h"

#include <memory>
#include <string>
#include <vector>

namespace calib {

enum class CalibrationBoardType {
    FanucIRvision,
    // Halcon,
    // ChArUco,
    // ChessBoard,
};

// =====================================================================
// CalibrationBoardFactory
// =====================================================================
//
// Centralised allocator for CalibrationBoard subclasses. Returns base-class
// std::unique_ptr<CalibrationBoard> so callers can stay board-agnostic. When
// type-specific accessors are needed (e.g. FanucIRvisionBoard::targetIndices),
// construct the subclass directly instead of going through the factory.
//
class CalibrationBoardFactory
{
public:
    // Default-construct a board of the given type using that type's own defaults.
    static std::unique_ptr<CalibrationBoard> create(CalibrationBoardType type);

    // Construct a FanucIRvisionBoard with explicit params.
    static std::unique_ptr<CalibrationBoard>
        createFanucIRvision(const FanucIRvisionBoard::Params& params = FanucIRvisionBoard::Params{});

    // Construct a board from a named preset. Returns nullptr for unknown names.
    // Recognised presets:
    //   "iRvision5mm", "iRvision11mm", "iRvision15mm", "iRvision22mm", "iRvision30mm"
    static std::unique_ptr<CalibrationBoard> createFromPreset(const std::string& presetName);

    // List of preset names recognised by createFromPreset().
    static std::vector<std::string> availablePresets();

    // Reconstruct a board instance from JSON produced by CalibrationBoard::toJson().
    // Dispatches on the top-level "type" field. Returns nullptr if the type is
    // unknown or the parsed Params fail validation.
    static std::unique_ptr<CalibrationBoard> createFromJson(const std::string& json);
};

} // namespace calib

#endif // CALIBRATION_BOARD_FACTORY_H
