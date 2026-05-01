#ifndef MC_CONTEXT_FACTORY_H
#define MC_CONTEXT_FACTORY_H

#include "mc_context_3e.h"

namespace vc::device {

namespace Factory {

/**
 * @brief contextFactory: allocate MC protocol context instance
 * @param frame_type
 * @return
 */
[[maybe_unused]] static std::shared_ptr<McContext> contextFactory(McFrameType frame_type, McDataCode data_code = McDataCode::Binary) {
    switch (frame_type) {
    case McFrameType::Frame_1C:
        return nullptr;
    case McFrameType::Frame_1E:
        return nullptr;
    case McFrameType::Frame_3E:
        return std::make_shared<Context_Mc3E>(data_code);
    case McFrameType::Frame_3C:
        return nullptr;
    default:
        return nullptr;
    }
}

}

}


#endif // MC_CONTEXT_FACTORY_H
