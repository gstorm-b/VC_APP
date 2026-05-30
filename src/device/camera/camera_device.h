#ifndef CAMERA_DEVICE_H
#define CAMERA_DEVICE_H

#include "device/idevice.h"
#include "calibration/calibrator.h"
#include <opencv2/opencv.hpp>

#define CAM_TYPE_REALSENSE       "Realsense"
#define CAM_TYPE_BASLER_GIGE     "Basler_GigE"
#define CAM_TYPE_BASLER_USB      "Basler_USB"

namespace vc::device {

enum CameraType {
    CamType,
    Realsense,
    BaslerGigE,
    BaslerUSB,
};

[[maybe_unused]] static QString CameraTypeToString(CameraType t) {
    switch(t) {
    case vc::device::CameraType::Realsense:
        return CAM_TYPE_REALSENSE;
    case vc::device::CameraType::BaslerGigE:
        return CAM_TYPE_BASLER_GIGE;
    case vc::device::CameraType::BaslerUSB:
        return CAM_TYPE_BASLER_USB;
    case CamType:
        return "";
    }
    return "";
};

[[maybe_unused]] static CameraType CameraTypeFromString(QString t) {
    if (t == CAM_TYPE_REALSENSE) {
        return CameraType::Realsense;
    } else if (t == CAM_TYPE_BASLER_GIGE) {
        return CameraType::BaslerGigE;
    } else if (t == CAM_TYPE_BASLER_USB) {
        return CameraType::BaslerUSB;
    }
    return CameraType::CamType;
}

class CameraCfg : public IDeviceCfg {
public:
    virtual void enableBlackLightControl(bool ena) = 0;

    virtual void exposureTimeLimit(double &min, double &max) = 0;
    virtual void setExposureTimeLimit(double min, double max) = 0;
    virtual double exposureTime() = 0;
    virtual void setExposureTime(double value) = 0;

    virtual void gainLimit(int &min, int &max) = 0;
    virtual void setGainLimit(int min, int max) = 0;
    virtual int gain() = 0;
    virtual void setGain(int value) = 0;

    virtual void acquisitionFrameRate(bool &enable, double &rate) = 0;
    virtual void setAcquisitionFrameRate(bool enable, double rate) = 0;

    virtual CameraType cameraType() const = 0;

    virtual QJsonObject toJson() const override {
        QJsonObject obj;
        obj[DEVICE_JSK_CAM_TYPE] = CameraTypeToString(this->cameraType());
        obj[DEVICE_JSK_CALIBRATOR] = QString::fromStdString(m_calibrator.toJson());
        obj[DEVICE_JSK_CALIB_BOARD_PRESET] = m_calibBoardPreset;
        return obj;
    }

    virtual bool fromJson(const QJsonObject &obj) override {
        if (!obj.contains(DEVICE_JSK_CALIBRATOR)) {
            LOG_USER_WARN << "Camera load calibrator failed.";
        } else {
            std::string calib_str = obj[DEVICE_JSK_CALIBRATOR].toString().toStdString();
            m_calibrator.fromJson(calib_str);
        }

        if (obj.contains(DEVICE_JSK_CALIB_BOARD_PRESET)) {
            m_calibBoardPreset = obj[DEVICE_JSK_CALIB_BOARD_PRESET].toString();
        }

        if (cameraType() != CameraTypeFromString(obj[DEVICE_JSK_CAM_TYPE].toString())) {
            LOG_DEV_ERR << "Cannot convert camera parameters, wrong camera type";
            LOG_DEV_ERR << obj[DEVICE_JSK_CAM_TYPE].toString();
            return false;
        }
        return true;
    }

    virtual calib::Calibrator calibrator() const {
        return m_calibrator;
    }

    virtual void setCalibrator(calib::Calibrator calib)  {
        m_calibrator = calib;
    }

    virtual QString calibBoardPreset() const {
        return m_calibBoardPreset;
    }

    virtual void setCalibBoardPreset(const QString &preset) {
        m_calibBoardPreset = preset;
    }

protected:
    calib::Calibrator m_calibrator;
    // Stored preset name (resolved by calib::CalibrationBoardFactory). Default
    // matches iRVision 22.5 mm dot grid; UI lets the user change it.
    QString m_calibBoardPreset{"iRvision-22.5mm"};
};

struct GrabResult {
    cv::Mat frame;
    bool isGrabSuccess;
    QString msg;
};

class CameraDevice : public IDevice {
    Q_OBJECT

public:
    explicit CameraDevice(QString id, QString name, QObject* parent = nullptr)
        : IDevice(id, name, parent) {}

    virtual DeviceType deviceType() const override {
        return DeviceType::Camera;
    }

    virtual CameraType cameraType() const = 0;
    virtual bool hasIOPort() const = 0;
    virtual bool isRgbCamera() const = 0;
    virtual void setGrabTimeout(int ms) = 0;
    virtual bool setExposure(double exposure) = 0;
    virtual bool setGain(double gain) = 0;
    virtual void setBacklightControl(bool enable) = 0;
    virtual bool readIO(QString name) = 0;
    virtual bool writeIO(QString name, bool value) = 0;
    virtual bool applyParametersChange() = 0;

    virtual GrabResult grabSingleShot() = 0;
    virtual bool startAutoContinuousShot() = 0;
    virtual void stopAutoContinousShot() = 0;
    virtual bool startContinuousShot() = 0;
    virtual void stopContinuousShot() = 0;
    virtual GrabResult softwareTriggerShot() = 0;

    virtual bool pushRequest(IRequest *request) override {
        Q_UNUSED(request);
        return false;
    }

    virtual QJsonObject toJson() const override {
        QJsonObject obj = IDevice::toJson();
        obj.insert(DEVICE_JSK_CAM_TYPE, CameraTypeToString(this->cameraType()));
        return obj;
    }

signals:
    void grabFinished(vc::device::GrabResult result);
    void exposureChanged(double value);
    void gainChanged(double value);
    void backlightControlChanged(bool ena);
    void parametersApplied(bool isOk);
};



} // namespace vc::device

#endif // CAMERA_DEVICE_H
