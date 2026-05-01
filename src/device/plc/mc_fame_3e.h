#ifndef MC_FAME_3E_H
#define MC_FAME_3E_H

#include "device/plc/mc_frame_abstract.h"
#include "device/plc/mc_request.h"
#include "device/plc/mc_context_3e.h"

namespace vc::device {

class Frame3E : public MCFrameAbstract {
public:
    Frame3E();

    FrameReturnCode makeSendFrame(vc::device::MCRequest *request, McContext* ctx, QByteArray &data) override;
    FrameReturnCode parseReceiveFrame(vc::device::MCRequest *request, McContext* ctx, QByteArray &data) override;
    QString lastErrorDescription() override;

private:
    bool checkErrorStatus(QByteArray &data);
    void make_send_data(Context_Mc3E* ctx, QByteArray &byte);

    FrameReturnCode build_read_bit(char &device, int &start, int &amount, Context_Mc3E *ctx, QByteArray &data);
    FrameReturnCode build_write_bit(vc::device::MCRequest *request, Context_Mc3E *ctx, QByteArray &data);

    FrameReturnCode build_read_word(char &device, int &start, int &amount, Context_Mc3E *ctx, QByteArray &data);
    FrameReturnCode build_write_word(vc::device::MCRequest *request, Context_Mc3E *ctx, QByteArray &data);

    FrameReturnCode parse_read_bit(vc::device::MCRequest *request, Context_Mc3E* ctx, QByteArray &data);
    FrameReturnCode parse_read_bit_from_word(vc::device::MCRequest *request, Context_Mc3E* ctx, QByteArray &data);

    FrameReturnCode parse_read_word(vc::device::MCRequest *request, Context_Mc3E* ctx, QByteArray &data);

    // all write request are same
    FrameReturnCode parse_write(vc::device::MCRequest *request, Context_Mc3E* ctx, QByteArray &data);

private:
    QString m_last_error;
    int m_total_bit_word_len;
};

}

#endif // MC_FAME_3E_H
