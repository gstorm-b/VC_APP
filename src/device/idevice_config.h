#ifndef IDEVICE_CONFIG_H
#define IDEVICE_CONFIG_H

#include <QJsonObject>

#define DEVICE_TYPE_NAME_CAMERA     "Camera"
#define DEVICE_TYPE_NAME_MC_PTC_DV  "MC Protocol Device"
#define DEVICE_TYPE_NAME_PLC        "PLC"
#define DEVICE_TYPE_NAME_TCP_IP_DV  "TCP/IP Device"
#define DEVICE_TYPE_NAME_ROBOT      "Robot"

#define DEVICE_JSK_ID               "DeviceId"
#define DEVICE_JSK_NAME             "DeviceName"
#define DEVICE_JSK_TYPE             "DeviceType"
#define DEVICE_JSK_CONFIG           "DeviceConfig"
#define DEVICE_JSK_CAM_TYPE         "CameraType"

#define DEVICE_JSK_MC_FRAME         "McFrameType"
#define DEVICE_JSK_MC_CONTEXT       "McContext"
// #define DEVICE_JSK_PLC_BRAND        "PlcBrand"
// #define DEVICE_JSK_PLC_TYPE         "PlcType"

namespace vc::device {

enum DeviceType {
    UserType,
    Camera,
    McDevice,
    PLC,
    TCPIP_DEVICE,
    Robot
};

[[maybe_unused]] static QString DeviceTypeToString(DeviceType t) {
    switch(t) {
    case vc::device::DeviceType::UserType:
        return "";
    case vc::device::DeviceType::Camera:
        return DEVICE_TYPE_NAME_CAMERA;
    case vc::device::DeviceType::McDevice:
        return DEVICE_TYPE_NAME_MC_PTC_DV;
    case vc::device::DeviceType::TCPIP_DEVICE:
        return DEVICE_TYPE_NAME_TCP_IP_DV;
    case vc::device::DeviceType::Robot:
        return DEVICE_TYPE_NAME_ROBOT;
    default:
        return "";
    }
    return "";
}

[[maybe_unused]] static DeviceType DeviceTypeFromString(QString t) {
    if (t == DEVICE_TYPE_NAME_CAMERA) {
        return DeviceType::Camera;
    } else if (t == DEVICE_TYPE_NAME_MC_PTC_DV) {
        return DeviceType::McDevice;
    } else if (t == DEVICE_TYPE_NAME_TCP_IP_DV) {
        return DeviceType::TCPIP_DEVICE;
    } else if (t == DEVICE_TYPE_NAME_ROBOT) {
        return DeviceType::Robot;
    }
    return UserType;
}

/**
 * @brief The IDeviceCfg class abstract device config
 */
class IDeviceCfg {
public:
    virtual ~IDeviceCfg() = default;

    /**
     * @brief deviceType
     * @return type of device
     */
    virtual DeviceType deviceType() const = 0;

    /**
     * @brief toJson convert to json for save paramters
     * @return
     */
    virtual QJsonObject toJson() const = 0;

    virtual const QMetaObject &getMetaObject() const = 0;

    /**
     * @brief fromJson convert parameters from json
     * @param obj QJsonObject
     * @return
     */
    virtual bool fromJson(const QJsonObject &obj) = 0;

    /**
     * @brief allocate clone instance
     * @return
     */
    virtual IDeviceCfg* clone() = 0;
};

}



#endif // IDEVICE_CONFIG_H
