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
    ~TaskLocalization() override;

    TaskType taskType()  const override {
        return TaskType::LocalizationTask;
    }

    bool isValid() const override {
        return m_isValid;
    }

    bool isReachLimitOfDeviceType(vc::device::DeviceType t) const override;
    void beginRuntime(bool mergeToTaskThread = false) override;
    void endRuntime() override;
    void stopAll() override;

    void setTaskLocalizeConfig(const TaskLocalizeConfig &cfg);

    TaskLocalizeConfig taskLocalizeConfig() const {
        return m_config;
    }

    void startCommissionMatching(
        std::shared_ptr<mtc::MatchGroup> group, cv::Mat image, vc::model::CameraWorkspace workspace) {

        // Deep-copy the live group on the caller's (GUI) thread before handing
        // it to the matching worker thread. MatchGroup is non-QObject and
        // unlocked, so the worker must read an isolated snapshot — otherwise it
        // races concurrent pattern edits (addPattern/removePattern/
        // setPatternImage). See snapshotPatternGroup().
        emit startCommissionMatchingRequest(
            snapshotPatternGroup(group), image.clone(), workspace);
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

    void onRuntimeCycleStarted(const QString &message);
    void onRuntimeRecovering(const QString &message);
    void onRuntimeReady(const QString &message);
    void onRuntimeFault(const QString &message);

private:
    void createRuntimeController();
    void destroyRuntimeController();
    void wireRuntimeControllerSignals();
    void wireMatchingWorkerSignals();
    LocalizationRuntimeController::RuntimeContext buildRuntimeContext() const;

    // Deep-copies a pattern group (group config + per-pattern config incl. the
    // raw training image) into an independent MatchGroup. Worker threads read
    // this snapshot instead of the live PatternGroupManager group, so GUI-thread
    // mutations cannot race the matcher. Used by both the runtime context build
    // and the commission matching path. Returns nullptr for a null source.
    static std::shared_ptr<mtc::MatchGroup>
    snapshotPatternGroup(const std::shared_ptr<mtc::MatchGroup> &source);

    LocalizationRuntimeController::SetupResult setupRuntimeController();
    void queueConfigureRuntimeController();
    void queueSetActiveCameraNumber(int number);
    void queueSetActivePatternGroupNumber(int number);
    void queueHandlePlcValues(const QMap<QString, QVariant> &values);

signals:
    void startPLCRequest();
    void startCameraRequest();
    void commissionMatchingFinished(mtc::MatchResult result);
    void cycleResultUpdated(vc::model::LocalizationRuntimeController::CycleResult result);
    void taskLogAppended(vc::model::LocalizationRuntimeController::TaskLogEntry entry);
    void cameraChanged(QString name);
    void patternChanged(QString name);

private:
    signals:
        void startCommissionMatchingRequest(std::shared_ptr<mtc::MatchGroup> group, cv::Mat image, CameraWorkspace workspace);

private:
    // ── Helpers for typed runner access ───────────────────────────────────────
    // Returns nullptr if the device isn't registered or is wrong type.
    vc::runtime::CameraRunner *cameraRunner(const QString &deviceId) const;
    vc::runtime::PlcRunner    *plcRunner(const QString &deviceId)    const;
    QThread *matchingRunner{nullptr};
    QObject *m_matchingWorker{nullptr};

public:
    static constexpr int kLimitCommDevice = 1;
    static constexpr int kLimitVisionOutputDevice = 1;
    static constexpr int kLimitNumCamera = 16;

private:
    QMap<device::DeviceType, int> m_limitDeviceMap;

    bool m_isValid{false};

    TaskLocalizeConfig m_config;

    // Device objects are retrieved from DeviceManager via taskRunner().
    // Typed cached pointers below are populated in setupTask() after
    // commission has confirmed which deviceId plays each role.
    QString m_plcDeviceId;

    LocalizationPipeline m_pipeline;
    LocalizationRuntimeController *m_runtimeController{nullptr};
    mtc::PatternGroupManager *m_patternManager;

    // Persistent runtime matcher — built once and reused across cycles (touched
    // only on the matchingRunner thread). The learned model is reloaded only
    // when the active pattern group changes (m_loadedRuntimeGroup identity).
    std::unique_ptr<mtc::ImageMatcher> m_runtimeMatcher;
    std::shared_ptr<mtc::MatchGroup> m_loadedRuntimeGroup;
};


}


Q_DECLARE_METATYPE(mtc::MatchResult)
Q_DECLARE_METATYPE(cv::Mat)
Q_DECLARE_METATYPE(std::shared_ptr<mtc::MatchGroup>)

#endif // TASK_LOCALIZATION_H
