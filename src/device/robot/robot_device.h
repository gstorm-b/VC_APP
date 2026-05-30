#ifndef ROBOT_DEVICE_H
#define ROBOT_DEVICE_H

#include "device/idevice.h"

#define ROBOT_TYPE_KAWASAKI   "Kawasaki"
#define ROBOT_TYPE_NACHI      "Nachi"
#define ROBOT_TYPE_HUAYAN     "Huayan"

namespace vc::device {

// Top-level dispatch handle for the robot family. Matches the Camera /
// PLC sub-type pattern: each vendor implementation registers a new value
// here; DeviceFactory::createRobotDevice() switches on this enum to pick
// the concrete subclass.
enum RobotType {
    RobotTypeNone,
    Kawasaki,
    Nachi,
    Huayan,    // placeholder for future vendor
};

[[maybe_unused]] static QString RobotTypeToString(RobotType t) {
    switch (t) {
    case RobotType::Kawasaki: return ROBOT_TYPE_KAWASAKI;
    case RobotType::Nachi:    return ROBOT_TYPE_NACHI;
    case RobotType::Huayan:   return ROBOT_TYPE_HUAYAN;
    case RobotType::RobotTypeNone:
        return "";
    }
    return "";
}

[[maybe_unused]] static RobotType RobotTypeFromString(QString t) {
    if (t == ROBOT_TYPE_KAWASAKI) return RobotType::Kawasaki;
    if (t == ROBOT_TYPE_NACHI)    return RobotType::Nachi;
    if (t == ROBOT_TYPE_HUAYAN)   return RobotType::Huayan;
    return RobotType::RobotTypeNone;
}

// =====================================================================
// RobotCfg — abstract config for the robot family
// =====================================================================
//
// Per Rule 12.5, the abstract base only carries the family-level dispatch
// (RobotType) and the family-level JSON header. Vendor-specific fields
// live in the concrete subclass — kept intentionally empty here until
// the first real vendor implementation lands.
class RobotCfg : public IDeviceCfg {
public:
    virtual RobotType robotType() const = 0;

    DeviceType deviceType() const override {
        return DeviceType::Robot;
    }

    QJsonObject toJson() const override {
        QJsonObject obj;
        obj[DEVICE_JSK_ROBOT_TYPE] = RobotTypeToString(this->robotType());
        return obj;
    }

    bool fromJson(const QJsonObject &obj) override {
        if (!obj.contains(DEVICE_JSK_ROBOT_TYPE)) {
            LOG_DEV_ERR << "RobotCfg: missing RobotType key";
            return false;
        }
        if (robotType() != RobotTypeFromString(obj[DEVICE_JSK_ROBOT_TYPE].toString())) {
            LOG_DEV_ERR << "RobotCfg: robot type mismatch -"
                        << obj[DEVICE_JSK_ROBOT_TYPE].toString();
            return false;
        }
        return true;
    }
};

// =====================================================================
// RobotDevice — abstract device for the robot family
// =====================================================================
//
// Minimum surface area as required by IDevice. Vendor-specific motion /
// teach-pendant / IO APIs are intentionally not declared here — they will
// be added once the first concrete vendor lands and the shared abstraction
// becomes concrete (Rule 12.5).
class RobotDevice : public IDevice {
    Q_OBJECT

public:
    explicit RobotDevice(QString id, QString name, QObject* parent = nullptr)
        : IDevice(id, name, parent) {}

    DeviceType deviceType() const override {
        return DeviceType::Robot;
    }

    virtual RobotType robotType() const = 0;

    QJsonObject toJson() const override {
        QJsonObject obj = IDevice::toJson();
        obj.insert(DEVICE_JSK_ROBOT_TYPE, RobotTypeToString(this->robotType()));
        return obj;
    }
};

} // namespace vc::device

#endif // ROBOT_DEVICE_H
