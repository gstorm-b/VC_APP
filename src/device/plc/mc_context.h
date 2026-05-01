#ifndef MC_CONTEXT_H
#define MC_CONTEXT_H

#include <QByteArray>
#include <QJsonObject>
#include <QObject>
#include <QMetaType>
#include <qtmetamacros.h>
#include "device/plc/mc_define.h"
#include "device/plc/mc_device_map.h"
#include "device/plc/mc_msg_interface.h"
#include "device/plc/mc_msg_tcp_client.h"

using namespace vc::device::mc;

namespace vc::device {

class McContext {
    Q_GADGET

    // --- Metadata for UI UI (Min/Max) ---
    Q_PROPERTY(vc::device::mc::McFrameType frameType READ frameType CONSTANT)
    Q_PROPERTY(vc::device::mc::McMsgItfType msgItfType READ msgIntefaceType CONSTANT)
    Q_PROPERTY(vc::device::mc::McDataCode dataCode READ dataCode CONSTANT)

    Q_PROPERTY(int refreshInterval READ refreshInterval WRITE setRefreshInterval)
    Q_PROPERTY(int activeMDevice READ activeMDevice WRITE setActiveMDevice)
    Q_PROPERTY(int startMAddress READ startMAddress WRITE setStartMAddress)
    Q_PROPERTY(int amountMAddress READ amountMAddress WRITE setAmountMAddress)
    Q_PROPERTY(int startDAddress READ startDAddress WRITE setStartDAddress)
    Q_PROPERTY(int amountDAddress READ amountDAddress WRITE setAmountDAddress)

    // --- Metadata for UI ---
    Q_CLASSINFO("frameType_name", "Frame")
    Q_CLASSINFO("msgItfType_name", "Interface")
    Q_CLASSINFO("dataCode_name", "Data Code")
    Q_CLASSINFO("refreshInterval_name", "Refresh Interval")
    Q_CLASSINFO("activeMDevice_name", "Active M Device")
    Q_CLASSINFO("startMAddress_name", "Start M-Address")
    Q_CLASSINFO("amountMAddress_name", "Amount M-Devices")
    Q_CLASSINFO("startDAddress_name", "Start D-Address")
    Q_CLASSINFO("amountDAddress_name", "Amount D-Devices")

    Q_CLASSINFO("refreshInterval_min", "10")
    Q_CLASSINFO("refreshInterval_max", "10000")

    Q_CLASSINFO("activeMDevice_min", "0")
    Q_CLASSINFO("activeMDevice_max", "65535")

    Q_CLASSINFO("startMAddress_min", "0")
    Q_CLASSINFO("startMAddress_max", "65535")

    Q_CLASSINFO("amountMAddress_min", "1")
    Q_CLASSINFO("amountMAddress_max", "1024")

    Q_CLASSINFO("startDAddress_min", "0")
    Q_CLASSINFO("startDAddress_max", "65535")

    Q_CLASSINFO("amountDAddress_min", "1")
    Q_CLASSINFO("amountDAddress_max", "1024")

public:
    // McContext() {};
    virtual ~McContext() = default;

    // --- abstract getter ---
    virtual const QMetaObject &getMetaObject() const = 0;
    virtual McFrameType frameType() const = 0;
    virtual McMsgItfType msgIntefaceType() const = 0;

    // --- Getters ---
    McDataCode dataCode() const { return m_data_code; }
    int activeMDevice() const { return m_activeMDevice; }
    int refreshInterval() const { return m_refreshInterval; }
    int startMAddress() const { return m_startMAddress; }
    int amountMAddress() const { return m_amountMAddress; }
    int startDAddress() const { return m_startDAddress; }
    int amountDAddress() const { return m_amountDAddress; }

    // --- Setters ---
    void setActiveMDevice(int val) { m_activeMDevice = val; }
    void setRefreshInterval(int val) { m_refreshInterval = val; }
    void setStartMAddress(int val) { m_startMAddress = val; }
    void setAmountMAddress(int val) { m_amountMAddress = val; }
    void setStartDAddress(int val) { m_startDAddress = val; }
    void setAmountDAddress(int val) { m_amountDAddress = val; }

    virtual McContext* clone() const = 0;

    virtual QJsonObject toJson() const {
        QJsonObject obj;
        obj["dataCode"] = qenumToString(m_data_code);
        obj["refreshInterval"] = m_refreshInterval;
        obj["activeMDevice"] = m_activeMDevice;
        obj["startMAddress"] = m_startMAddress;
        obj["amountMAddress"] = m_amountMAddress;
        obj["startDAddress"] = m_startDAddress;
        obj["amountDAddress"] = m_amountDAddress;
        return obj;
    }

    virtual bool fromJson(const QJsonObject &obj) {
        m_data_code = stringToQEnum(obj["dataCode"].toString(), McDataCode::DataCode_User);
        m_refreshInterval = obj["refreshInterval"].toInt(m_refreshInterval);
        m_activeMDevice = obj["activeMDevice"].toInt(m_activeMDevice);
        m_startMAddress  = obj["startMAddress"].toInt(m_startMAddress);
        m_amountMAddress = obj["amountMAddress"].toInt(m_amountMAddress);
        m_startDAddress  = obj["startDAddress"].toInt(m_startDAddress);
        m_amountDAddress = obj["amountDAddress"].toInt(m_amountDAddress);
        if (m_data_code == McDataCode::DataCode_User) {
            m_data_code = McDataCode::Ascii;
            return false;
        }
        return true;
    }

    virtual McMsgItfConfig* msgConfig() const {
        return m_msg_cfg.get();
    }

    void setDeviceMap(McDeviceMap* dv_map) {
        if (dv_map == nullptr) {
            return;
        }

        this->m_device_map = dv_map;
    }

    McDeviceMap* getDeviceMap() {
        return this->m_device_map;
    }

public:
    int m_refreshInterval{100};
    int m_activeMDevice{2000};
    int m_startMAddress{2000};
    int m_amountMAddress{64};
    int m_startDAddress{2000};
    int m_amountDAddress{64};

protected:
    McDataCode m_data_code;
    std::shared_ptr<McMsgItfConfig> m_msg_cfg;
    McDeviceMap* m_device_map{nullptr};
};

}

Q_DECLARE_METATYPE(vc::device::McContext)

#endif // MC_CONTEXT_H
