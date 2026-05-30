#ifndef IDEVICE_CONFIG_H
#define IDEVICE_CONFIG_H

#include <QJsonObject>

#define DEVICE_TYPE_NAME_CAMERA          "Camera"
#define DEVICE_TYPE_NAME_PLC             "PLC"
#define DEVICE_TYPE_NAME_TCP_IP_DV       "TCP/IP Device"
#define DEVICE_TYPE_NAME_VISION_OUTPUT   "Vision Output Device"
#define DEVICE_TYPE_NAME_ROBOT           "Robot"

#define DEVICE_JSK_ID               "DeviceId"
#define DEVICE_JSK_NAME             "DeviceName"
#define DEVICE_JSK_TASKID           "AssignedTaskId"
#define DEVICE_JSK_TYPE             "DeviceType"
#define DEVICE_JSK_CONFIG           "DeviceConfig"
#define DEVICE_JSK_CAM_TYPE             "CameraType"
#define DEVICE_JSK_CALIBRATOR           "CameraCalibrator"
#define DEVICE_JSK_CALIB_BOARD_PRESET   "CalibBoardPreset"

#define DEVICE_JSK_ROBOT_TYPE           "RobotType"

#define DEVICE_JSK_PLC_TYPE         "PlcType"
#define DEVICE_JSK_MC_FRAME         "McFrameType"
#define DEVICE_JSK_MC_CONTEXT       "McContext"

#define DEVICE_JSK_VOUT_TYPE               "VisionOutputType"
#define DEVICE_JSK_VOUT_LISTEN_ADDR        "ListenAddress"
#define DEVICE_JSK_VOUT_MAIN_PORT          "MainPort"
#define DEVICE_JSK_VOUT_HB_PORT            "HeartbeatPort"
#define DEVICE_JSK_VOUT_HB_INTERVAL        "HeartbeatIntervalMs"
#define DEVICE_JSK_VOUT_HB_TIMEOUT         "HeartbeatTimeoutMs"
// #define DEVICE_JSK_PLC_BRAND        "PlcBrand"
// #define DEVICE_JSK_PLC_TYPE         "PlcType"

namespace vc::device {

enum DeviceType {
    UserType,
    Camera,
    PLC,
    VisionOutput,
    Robot
};

[[maybe_unused]] static QString DeviceTypeToString(DeviceType t) {
    switch(t) {
    case vc::device::DeviceType::UserType:
        return "";
    case vc::device::DeviceType::Camera:
        return DEVICE_TYPE_NAME_CAMERA;
    case vc::device::DeviceType::PLC:
        return DEVICE_TYPE_NAME_PLC;
    case vc::device::DeviceType::VisionOutput:
        return DEVICE_TYPE_NAME_VISION_OUTPUT;
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
    } else if (t == DEVICE_TYPE_NAME_PLC) {
        return DeviceType::PLC;
    } else if (t == DEVICE_TYPE_NAME_VISION_OUTPUT) {
        return DeviceType::VisionOutput;
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
