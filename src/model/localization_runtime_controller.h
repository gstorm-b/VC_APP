#ifndef LOCALIZATION_RUNTIME_CONTROLLER_H
#define LOCALIZATION_RUNTIME_CONTROLLER_H

#include <QObject>
#include <QHash>
#include <QDateTime>
#include <QMetaObject>
#include <QPointer>
#include <QMap>
#include <QStringList>
#include <QVariant>
#include <QVector>

#include <memory>

#include <opencv2/core/mat.hpp>

#include "device/idevice_config.h"
#include "device/camera/camera_device.h"
#include "device/output_device/vision_output_config.h"
#include "device/output_device/vision_output_request.h"
#include "matching/image_matcher.h"
#include "matching/robot_picking_checker.h"
#include "model/localization_fault_code.h"
#include "model/localization_recovery_policy.h"
#include "model/localization_signal_mapper.h"
#include "model/task_localization_config.h"
#include "runtime/device_command.h"

namespace vc::runtime {
class CameraRunner;
class IDeviceRunner;
class PlcRunner;
class VisionOutputRunner;
}

namespace vc::model {

class LocalizationRuntimeController : public QObject {
    Q_OBJECT

public:
    struct RuntimeContext {
        TaskLocalizeConfig config;
        QString primaryPlcDeviceId;
        QString visionOutputDeviceId;
        QPointer<vc::runtime::PlcRunner> primaryPlcRunner;
        QPointer<vc::runtime::VisionOutputRunner> visionOutputRunner;
        QMap<int, QString> cameraDeviceIds;
        QMap<int, QPointer<vc::runtime::CameraRunner>> cameraRunners;
        QMap<int, std::shared_ptr<mtc::MatchGroup>> patternGroups;
        QMap<int, calib::Calibrator> cameraCalibrators;
        // Robot kinematic check settings snapshotted from the assigned vision
        // output device; drives the per-object robotPossiblePickingCheck.
        vc::device::RobotKinematicCheckConfig robotCheckConfig;
        CameraWorkspace activeCameraWorkspace;
        int activeCameraNumber{-1};
        int activePatternGroupNumber{-1};
    };

    struct ResultRow {
        int index{0};
        QString patternName;
        double score{0.0};
        double imageX{0.0};
        double imageY{0.0};
        double imageR{0.0};
        vc::device::VisionOutputPosition world;
        QString status;
    };

    struct CycleResult {
        bool faulted{false};
        LocalizationFaultCode faultCode{LocalizationFaultCode::None};
        int detectedNumber{0};
        int sentNumber{0};
        double matchingTimeMs{0.0};
        bool lowArea{false};
        cv::Mat rawImage;
        cv::Mat displayImage;
        mtc::MatchResult matchResult;
        QVector<ResultRow> rows;
    };

    struct TaskLogEntry {
        QDateTime timestamp;
        QString severity;
        QString message;
    };

    struct SetupResult {
        bool valid{false};
        QString primaryPlcDeviceId;
        QString visionOutputDeviceId;
        QStringList errors;
    };

    explicit LocalizationRuntimeController(QObject *parent = nullptr);

    void configure(const TaskLocalizeConfig &config);
    void setActiveCameraNumber(int cameraNumber);
    void setActivePatternGroupNumber(int patternGroupNumber);
    void setRecoveryPolicies(const LocalizationRecoveryPolicy &cameraPolicy,
                             const LocalizationRecoveryPolicy &plcPolicy,
                             const LocalizationRecoveryPolicy &visionOutputPolicy);

    SetupResult setup(const RuntimeContext &context);
    void execute();
    bool isValid() const { return m_valid; }

    void handlePlcValues(const QMap<QString, QVariant> &values);
    void onRuntimeMatchingFinished(int cycleId, mtc::MatchResult matchResult);

signals:
    void signalChanged(QString name, QVariant value);
    void cameraNumberChanged(QVariant value);
    void patternNumberChanged(QVariant value);
    void cycleResultUpdated(vc::model::LocalizationRuntimeController::CycleResult result);
    void taskLogAppended(vc::model::LocalizationRuntimeController::TaskLogEntry entry);
    void runtimeCycleStarted(QString message);
    void runtimeRecovering(QString message);
    void runtimeReady(QString message);
    void runtimeFault(QString message);
    void runtimeMatchingRequested(int cycleId,
                                  std::shared_ptr<mtc::MatchGroup> group,
                                  CameraWorkspace workspace,
                                  cv::Mat image,
                                  std::shared_ptr<mtc::IRobotPickingChecker> pickingChecker);

private:
    struct RoleRecoveryContext {
        QString roleName;
        QString deviceId;
        QPointer<vc::runtime::IDeviceRunner> runner;
        LocalizationRecoveryPolicy policy;
        int retryCount{0};
        bool retryScheduled{false};
        bool faultRaised{false};
    };

    enum class RunnerRole {
        Camera,
        PrimaryPlc,
        VisionOutput
    };

    enum class CycleState {
        NotReady,
        ReadyForTrigger,
        Running,
        WaitingTriggerReset,
        Recovering,
        Faulted
    };

    void resetRuntimeBindings();
    void bindFixedRoleRunners();
    void bindActiveCameraRole(int cameraNumber);
    void bindRoleContext(RunnerRole role,
                         vc::runtime::IDeviceRunner *runner,
                         const QString &deviceId,
                         const LocalizationRecoveryPolicy &policy);
    void clearRoleContext(RunnerRole role);
    static int roleKey(RunnerRole role) { return static_cast<int>(role); }
    void handleRoleStatusChanged(RunnerRole role, vc::device::ConnectStatus status);
    void scheduleRoleReconnect(RunnerRole role);
    void requestRoleConnectNow(RunnerRole role);
    void raiseRoleFault(RunnerRole role, vc::device::ConnectStatus status);
    bool allRequiredRolesHealthy() const;
    void markRuntimeReady(const QString &message);
    void publishBoolSignal(const QString &name, bool value);
    void publishNumberSignal(const QString &name, int value);
    void publishInitialReadyOutputs();
    void publishCycleStartOutputs();
    void publishCycleSuccessOutputs(const CycleResult &result);
    void publishCycleFaultOutputs(LocalizationFaultCode code);
    void appendTaskLog(const QString &severity, const QString &message);
    bool validateActivePatternGroup(QStringList *errors = nullptr) const;
    bool validateActiveCameraCalibration(QStringList *errors = nullptr) const;
    // (Re)build the robot-pickability checker for the active camera. Called once
    // at runtime setup and again on active-camera change (calibrator differs per
    // camera). Null when the kinematic check is disabled or no valid calibration.
    void rebuildPickingChecker();
    std::shared_ptr<mtc::MatchGroup> snapshotActivePatternGroup() const;
    QVector<vc::device::VisionOutputPosition> buildVisionOutputPositions(
        const mtc::MatchResult &matchResult,
        QVector<ResultRow> *rows,
        LocalizationFaultCode *faultCode) const;
    vc::runtime::CameraRunner *activeCameraRunner() const;
    vc::runtime::PlcRunner *primaryPlcRunner() const;
    vc::runtime::VisionOutputRunner *visionOutputRunner() const;
    void startCycle();
    void abortCycle(LocalizationFaultCode code, const QString &message);

private slots:
    void onCameraGrabFinished(vc::device::GrabResult result);
    void onCameraCommandFinished(vc::runtime::DeviceCommandResult result);
    void onVisionOutputResultFinished(bool ok, const QString &message);
    void onPrimaryPlcStatusChanged(vc::device::ConnectStatus status);
    void onVisionOutputStatusChanged(vc::device::ConnectStatus status);
    void onCameraStatusChanged(vc::device::ConnectStatus status);
    void onPrimaryPlcError(const QString &message);
    void onVisionOutputError(const QString &message);
    void onCameraError(const QString &message);

private:
    RuntimeContext m_context;
    TaskLocalizeConfig m_config;
    LocalizationSignalMapper m_signalMapper;
    QMetaObject::Connection m_plcValueConnection;
    QMetaObject::Connection m_cameraGrabConnection;
    QMetaObject::Connection m_cameraCommandConnection;
    QMetaObject::Connection m_visionOutputResultConnection;
    bool m_valid{false};
    int m_activeCameraNumber{-1};
    // Robot-pickability checker for the active camera (built at setup / camera
    // change). Passed to the matcher each cycle; null = matching not gated.
    std::shared_ptr<mtc::IRobotPickingChecker> m_pickingChecker;
    CameraWorkspace m_activeCameraWorkspace;
    int m_activePatternGroupNumber{-1};
    int m_activeCycleId{0};
    bool m_lastExecuteTrigger{false};
    CycleState m_cycleState{CycleState::NotReady};
    CycleResult m_pendingCycleResult;
    LocalizationRecoveryPolicy m_cameraRecoveryPolicy{defaultCameraRecoveryPolicy()};
    LocalizationRecoveryPolicy m_plcRecoveryPolicy{defaultPlcRecoveryPolicy()};
    LocalizationRecoveryPolicy m_visionOutputRecoveryPolicy{defaultVisionOutputRecoveryPolicy()};
    QHash<int, RoleRecoveryContext> m_recoveryContexts;
};

} // namespace vc::model

Q_DECLARE_METATYPE(vc::model::LocalizationRuntimeController::ResultRow)
Q_DECLARE_METATYPE(vc::model::LocalizationRuntimeController::CycleResult)
Q_DECLARE_METATYPE(vc::model::LocalizationRuntimeController::TaskLogEntry)
Q_DECLARE_METATYPE(std::shared_ptr<mtc::IRobotPickingChecker>)

#endif // LOCALIZATION_RUNTIME_CONTROLLER_H
