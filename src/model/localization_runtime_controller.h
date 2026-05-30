#ifndef LOCALIZATION_RUNTIME_CONTROLLER_H
#define LOCALIZATION_RUNTIME_CONTROLLER_H

#include <QObject>
#include <QHash>
#include <QMetaObject>
#include <QPointer>
#include <QStringList>
#include <QVariant>

#include "device/idevice_config.h"
#include "model/localization_recovery_policy.h"
#include "model/localization_signal_mapper.h"
#include "model/task_localization_config.h"

namespace vc::runtime {
class IDeviceRunner;
}

namespace vc::model {

class LocalizationPipeline;
class TaskLocalization;

class LocalizationRuntimeController : public QObject {
    Q_OBJECT

public:
    struct SetupResult {
        bool valid{false};
        QString primaryPlcDeviceId;
        QString visionOutputDeviceId;
        QStringList errors;
    };

    explicit LocalizationRuntimeController(QObject *parent = nullptr);

    void setPipeline(LocalizationPipeline *pipeline);
    void configure(const TaskLocalizeConfig &config);
    void setActiveCameraNumber(int cameraNumber);
    void setRecoveryPolicies(const LocalizationRecoveryPolicy &cameraPolicy,
                             const LocalizationRecoveryPolicy &plcPolicy,
                             const LocalizationRecoveryPolicy &visionOutputPolicy);

    SetupResult setup(TaskLocalization *task);
    void execute();
    bool isValid() const { return m_valid; }

    void handlePlcValues(const QMap<QString, QVariant> &values);

signals:
    void signalChanged(QString name, QVariant value);
    void cameraNumberChanged(QVariant value);
    void patternNumberChanged(QVariant value);
    void runtimeRecovering(QString message);
    void runtimeReady(QString message);
    void runtimeFault(QString message);

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

    bool validateDeviceRole(TaskLocalization *task,
                            const QString &deviceId,
                            const QString &roleName,
                            vc::device::DeviceType expectedType,
                            QStringList *errors) const;
    void resetRuntimeBindings();
    void bindFixedRoleRunners(TaskLocalization *task);
    void bindActiveCameraRole(TaskLocalization *task, int cameraNumber);
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

private slots:
    void onPrimaryPlcStatusChanged(vc::device::ConnectStatus status);
    void onVisionOutputStatusChanged(vc::device::ConnectStatus status);
    void onCameraStatusChanged(vc::device::ConnectStatus status);
    void onPrimaryPlcError(const QString &message);
    void onVisionOutputError(const QString &message);
    void onCameraError(const QString &message);

private:
    TaskLocalizeConfig m_config;
    LocalizationSignalMapper m_signalMapper;
    LocalizationPipeline *m_pipeline{nullptr};
    TaskLocalization *m_task{nullptr};
    QMetaObject::Connection m_plcValueConnection;
    bool m_valid{false};
    int m_activeCameraNumber{-1};
    LocalizationRecoveryPolicy m_cameraRecoveryPolicy{defaultCameraRecoveryPolicy()};
    LocalizationRecoveryPolicy m_plcRecoveryPolicy{defaultPlcRecoveryPolicy()};
    LocalizationRecoveryPolicy m_visionOutputRecoveryPolicy{defaultVisionOutputRecoveryPolicy()};
    QHash<int, RoleRecoveryContext> m_recoveryContexts;
};

} // namespace vc::model

#endif // LOCALIZATION_RUNTIME_CONTROLLER_H
