#ifndef MC_FRAME_ABSTRACT_H
#define MC_FRAME_ABSTRACT_H

#include <QByteArray>
#include "device/plc/mc_context.h"
#include "device/plc/mc_request.h"

namespace vc::device {

class MCFrameAbstract {
public:
    enum FrameReturnCode {
        RequestFrameOK,
        RequestFrameError,
        WaitingReceive,
        ResponseOk,
        ResponseError,
        ResponseInvalid,
        ObjectError
    };

    virtual ~MCFrameAbstract() = default;

    /**
     * @brief makeSendFrame
     * @param request
     * @param byte
     * @return return true if build frame success
     */
    virtual FrameReturnCode makeSendFrame(vc::device::MCRequest *request, McContext* ctx, QByteArray &byte) = 0;
    virtual FrameReturnCode parseReceiveFrame(vc::device::MCRequest *request, McContext* ctx, QByteArray &byte) = 0;
    virtual QString lastErrorDescription() = 0;
};

}

#endif // MC_FRAME_ABSTRACT_H
