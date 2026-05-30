#ifndef PLC_DEVICE_H
#define PLC_DEVICE_H

#include <memory>
#include <QMap>
#include "device/idevice.h"

#define PLC_TYPE_MITSUBISHI_MC   "MitsubishiMc"

namespace vc::device {

// Family-level sub-type dispatch handle for the PLC family. Concrete
// vendors register a value here; DeviceFactory::createPlcDevice() switches
// on this enum to pick the concrete subclass.
enum PlcType {
    PlcTypeNone,
    MitsubishiMc,
};

[[maybe_unused]] static QString PlcTypeToString(PlcType t) {
    switch (t) {
    case PlcType::MitsubishiMc:  return PLC_TYPE_MITSUBISHI_MC;
    case PlcType::PlcTypeNone:
        return "";
    }
    return "";
}

[[maybe_unused]] static PlcType PlcTypeFromString(QString t) {
    if (t == PLC_TYPE_MITSUBISHI_MC) return PlcType::MitsubishiMc;
    return PlcType::PlcTypeNone;
}

// =====================================================================
// PlcCfg — abstract config for the PLC family
// =====================================================================
//
// Carries only the family-level dispatch field. Concrete configs
// (McProtocolConfig for Mitsubishi MC, future OmronFinsConfig, …) inherit
// and add their protocol-specific Q_PROPERTYs.
class PlcCfg : public IDeviceCfg {
public:
    virtual PlcType plcType() const = 0;

    DeviceType deviceType() const override {
        return DeviceType::PLC;
    }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj[DEVICE_JSK_PLC_TYPE] = PlcTypeToString(this->plcType());
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        if (!obj.contains(DEVICE_JSK_PLC_TYPE)) {
            LOG_DEV_ERR << "PlcCfg: missing PlcType key";
            return false;
        }
        if (plcType() != PlcTypeFromString(obj[DEVICE_JSK_PLC_TYPE].toString())) {
            LOG_DEV_ERR << "PlcCfg: type mismatch -"
                        << obj[DEVICE_JSK_PLC_TYPE].toString();
            return false;
        }
        return true;
    }
};

class PlcValueMap {
public:
    virtual ~PlcValueMap() = default;
    virtual std::shared_ptr<PlcValueMap> clone() const = 0;
};

// =====================================================================
// PlcDevice — abstract base for the PLC family
// =====================================================================
//
// Concrete vendors (McProtocolDevice for Mitsubishi MC, future
// OmronFinsDevice, SiemensS7Device, …) inherit from this base. The base
// only carries the family-level dispatch (plcType()) and the family JSON
// header; vendor-specific surface lives entirely on the concrete subclass.
class PlcDevice : public IDevice {
    Q_OBJECT

public:
    explicit PlcDevice(QString id, QString name, QObject* parent = nullptr)
        : IDevice(id, name, parent) {}

    DeviceType deviceType() const override {
        return DeviceType::PLC;
    }

    virtual PlcType plcType() const = 0;

    QJsonObject toJson() const override {
        QJsonObject obj = IDevice::toJson();
        obj.insert(DEVICE_JSK_PLC_TYPE, PlcTypeToString(this->plcType()));
        return obj;
    }

signals:
    void pollingUpdate(std::shared_ptr<vc::device::PlcValueMap> value_map);
    void valueChanged(QMap<QString, QVariant> values);
};

} // namespace vc::device

#endif // PLC_DEVICE_H
