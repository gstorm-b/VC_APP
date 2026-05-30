#ifndef TASK_LOCALIZATION_H
#define TASK_LOCALIZATION_H

#include "model/itask.h"
#include "device/camera/camera_device.h"
#include "device/plc/mc_protocol_device.h"
#include "model/localization_pipeline.h"
#include "model/localization_runtime_controller.h"
#include "task_localization_config.h"
#include "matching/pattern_group_manager.h"
#include <opencv2/imgcodecs.hpp>

// Runtime runners (for typed access inside executeLocalization)
#include "runtime/camera_runner.h"
#include "runtime/plc_runner.h"

namespace vc::model {

class TaskLocalization : public ITask {
    Q_OBJECT

public:
    explicit TaskLocalization(QString name, QString id = "", QObject* parent = nullptr);

    TaskType taskType()  const override {
        return TaskType::LocalizationTask;
    }

    bool isValid() const override {
        return m_isValid;
    }

    bool isReachLimitOfDeviceType(vc::device::DeviceType t) const override;
    void beginRuntime(bool mergeToTaskThread = false) override;

    void setTaskLocalizeConfig(const TaskLocalizeConfig &cfg);

    TaskLocalizeConfig taskLocalizeConfig() const {
        return m_config;
    }

    void startCommissionMatching(
        std::shared_ptr<mtc::MatchGroup> group, cv::Mat &image) {

        emit startCommissionMatchingRequest(group, image.clone());
    }

    mtc::PatternGroupManager* patternManager() const {
        return m_patternManager;
    }

    // Image BLOB I/O — paired with the JSON written in toJson(). Each entry
    // in the returned map uses the key "g{groupNumber}_p{patternNumber}".
    // ProjectRepository stores these blobs in the project_images table and
    // re-injects them on load via loadTaskImageMap().
    QMap<QString, cv::Mat> getTaskImageMap() override;
    bool                   loadTaskImageMap(QMap<QString, cv::Mat> &mapping) override;

    QJsonObject toJson() const override;
    bool fromJson(const QJsonObject& obj) override;

    // Family-level typed access to the assigned PLC device via the runner.
    // Vendor-specific access (e.g. Mitsubishi frame type) is reached by
    // qobject_cast<McProtocolDevice *>(plcDevice()) at the call site.
    vc::device::PlcDevice *plcDevice() const;

public slots:
    void setupTask();
    void executeLocalization();
    void setCameraNumber(int number);
    void setPatternNumber(int number);

private slots:
    // ── Signals value change method ───────────────────────────────────────
    void onCommDeviceValueChanged(QMap<QString, QVariant> values);
    void onSignalChangeCameraNumber(QVariant value);
    void onSignalChangePatternNumber(QVariant value);

    void waitReconnectCameraHandle(device::ConnectStatus status);
    void selectedCameraConnectStatusChanged(device::ConnectStatus status);
    void onRuntimeRecovering(const QString &message);
    void onRuntimeReady(const QString &message);
    void onRuntimeFault(const QString &message);

private:
    void wireMatchingCommissionSignals();

signals:
    void startPLCRequest();
    void startCameraRequest();
    void commissionMatchingFinished(mtc::MatchResult result);
    void cameraChanged(QString name);
    void patternChanged(QString name);

private:
    signals:
        void startCommissionMatchingRequest(std::shared_ptr<mtc::MatchGroup> group, cv::Mat image);

private:
    // ── Helpers for typed runner access ───────────────────────────────────────
    // Returns nullptr if the device isn't registered or is wrong type.
    vc::runtime::CameraRunner *cameraRunner(const QString &deviceId) const;
    vc::runtime::PlcRunner    *plcRunner(const QString &deviceId)    const;
    QThread *matchingRunner{nullptr};

public:
    const int limit_comm_device = 1;
    const int limit_vision_output_device = 1;
    const int limit_num_camera = 16;

private:
    QMap<device::DeviceType, int> m_limitDeviceMap;

    bool m_isValid{false};

    TaskLocalizeConfig m_config;

    // Device objects are retrieved from DeviceManager via taskRunner().
    // Typed cached pointers below are populated in setupTask() after
    // commission has confirmed which deviceId plays each role.
    QString m_plcDeviceId;

    int m_currentCamNumber;
    int m_curentPatternNumber;
    std::shared_ptr<device::CameraDevice> m_selectedCamera;
    std::shared_ptr<device::CameraDevice> m_nextConnectCamera;

    mtc::MatchResult m_lastMatchResult;
    QString m_lastVisionOutput;

    LocalizationPipeline m_pipeline;
    LocalizationRuntimeController *m_runtimeController{nullptr};
    mtc::PatternGroupManager *m_patternManager;
};


}


#endif // TASK_LOCALIZATION_H
