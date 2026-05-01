#ifndef MC_REQUEST_H
#define MC_REQUEST_H

#include <memory>
#include <QString>
#include <QList>
#include "device/irequest.h"
#include "device/plc/memory_utils.h"

namespace vc::device {

struct McResult {
    bool isOk;
    int startAddress;
    QString register_type;
    int register_amount;
    QString msg;
    QByteArray data;
};

class MCRequest : public IRequest {
public:
    enum RqType {
        ReadBit,
        WriteBit,
        ReadWord,
        WriteWord,
    };

    MCRequest() {

    }

    RequestType type() const override {
        return RequestType::Request_MC;
    }

    MCRequest(RqType rqtype, QString head_device, int amount) {
        m_type = rqtype;
        if (head_device.isEmpty()) {
            is_config = false;
            return;
        }
        m_device_type = head_device[0].toLatin1();
        bool success;
        m_start_address = head_device.mid(1).toInt(&success, 10);
        if (!success) {
            is_config = false;
            return;
        }

        if ((amount <= 0) || (amount > 64)) {
            is_config = false;
            return;
        }

        m_amount = amount;
        is_config = true;
    }

    MCRequest(RqType rqtype, char dv_type, int start_address, int amount) {
        m_type = rqtype;
        m_device_type = dv_type;
        m_start_address = start_address;

        if ((amount <= 0) || (amount > 128)) {
            is_config = false;
            return;
        }

        m_amount = amount;
        is_config = true;
    }

    const bool isValid() {
        return is_config;
    }

    void buildWriteData_Bit_Device(QList<quint8> &value) {
        m_value.clear();
        for (int idx=0;idx<value.size();idx++) {
            m_value.append(static_cast<quint16>((value.at(idx) == 0x00) ? 0x00 : 0x01));
        }
    }

    void buildWriteData_Bit_Device(quint8 value) {
        m_value.clear();
        m_value.append(value);
    }

    void buildWriteData_Word_Device_Word(QList<qint16> &value) {
        m_value.clear();
        for (int idx=0;idx<value.size();idx++) {
            m_value.append(static_cast<quint16>(value.at(idx)));
        }
    }

    void buildWriteData_Word_Device_Word(qint16 value) {
        m_value.clear();
        m_value.append(static_cast<quint16>(value));
    }

    void buildWriteData_Word_Device_DoubleWord(QList<qint32> &value) {
        m_value.clear();
        for (int idx=0;idx<value.size();idx++) {
            m_value.append(static_cast<quint16>(value.at(idx) & 0xFFFF));
            m_value.append(static_cast<quint16>((value.at(idx) >> 16) & 0xFFFF));
        }
    }

    void buildWriteData_Word_Device_Float(QList<float> &value) {
        m_value.clear();
        for (int idx=0;idx<value.size();idx++) {
            quint32 fnum = floatToReal32(value.at(idx));
            m_value.append(static_cast<quint16>(fnum & 0xFFFF));
            m_value.append(static_cast<quint16>((fnum >> 16) & 0xFFFF));
        }
    }

    void buildWriteData_Word_Device_DoubleFloat(QList<double> &value) {
        m_value.clear();
        for (int idx=0;idx<value.size();idx++) {
            quint64 fnum = doubleToReal64(value.at(idx));
            m_value.append(static_cast<quint16>(fnum & 0xFFFF));
            m_value.append(static_cast<quint16>((fnum >> 16) & 0xFFFF));
            m_value.append(static_cast<quint16>((fnum >> 32) & 0xFFFF));
            m_value.append(static_cast<quint16>((fnum >> 48) & 0xFFFF));
        }
    }

    std::shared_ptr<IRequest> clone() const override {
        return std::make_shared<MCRequest>(*this);
    }

public:
    MCRequest::RqType m_type;
    char m_device_type;
    int m_start_address;
    int m_amount;
    QList<quint16> m_value;

private:
    bool is_config = false;
};

} // namespace vc::device


#endif // MC_REQUEST_H
