#ifndef LOCALIZATION_RECOVERY_POLICY_H
#define LOCALIZATION_RECOVERY_POLICY_H

#include <QString>

#include "device/idevice.h"

namespace vc::model {

enum class LocalizationRecoveryAction {
    Ignore,
    RetryScheduled,
    EscalateFault
};

struct LocalizationRecoveryPolicy {
    QString roleName;
    int maxRetries{10};
    int retryIntervalMs{5000};
    int connectTimeoutMs{3000};
    bool retryOnConnectFailed{true};
    bool retryOnLostConnected{true};

    bool isRecoverableStatus(vc::device::ConnectStatus status) const
    {
        if (status == vc::device::ConnectStatus::ConnectFailed) {
            return retryOnConnectFailed;
        }
        if (status == vc::device::ConnectStatus::LostConnected) {
            return retryOnLostConnected;
        }
        return false;
    }

    bool canRetry(int currentRetryCount) const
    {
        return currentRetryCount < maxRetries;
    }
};

inline LocalizationRecoveryPolicy defaultCameraRecoveryPolicy()
{
    LocalizationRecoveryPolicy policy;
    policy.roleName = QStringLiteral("camera");
    return policy;
}

inline LocalizationRecoveryPolicy defaultPlcRecoveryPolicy()
{
    LocalizationRecoveryPolicy policy;
    policy.roleName = QStringLiteral("primary_plc");
    return policy;
}

inline LocalizationRecoveryPolicy defaultVisionOutputRecoveryPolicy()
{
    LocalizationRecoveryPolicy policy;
    policy.roleName = QStringLiteral("vision_output");
    return policy;
}

inline LocalizationRecoveryAction decideRecoveryAction(
    const LocalizationRecoveryPolicy &policy,
    vc::device::ConnectStatus status,
    int currentRetryCount,
    bool retryAlreadyScheduled)
{
    if (!policy.isRecoverableStatus(status)) {
        return LocalizationRecoveryAction::Ignore;
    }

    if (retryAlreadyScheduled) {
        return LocalizationRecoveryAction::Ignore;
    }

    if (!policy.canRetry(currentRetryCount)) {
        return LocalizationRecoveryAction::EscalateFault;
    }

    return LocalizationRecoveryAction::RetryScheduled;
}

inline QString buildRecoveryFaultMessage(
    const LocalizationRecoveryPolicy &policy,
    const QString &deviceId,
    vc::device::ConnectStatus status,
    int performedRetries)
{
    QString statusText;
    switch (status) {
    case vc::device::ConnectStatus::LostConnected:
        statusText = QStringLiteral("LostConnected");
        break;
    case vc::device::ConnectStatus::ConnectFailed:
        statusText = QStringLiteral("ConnectFailed");
        break;
    case vc::device::ConnectStatus::NoConnection:
        statusText = QStringLiteral("NoConnection");
        break;
    case vc::device::ConnectStatus::Disconnected:
        statusText = QStringLiteral("Disconnected");
        break;
    case vc::device::ConnectStatus::Connected:
        statusText = QStringLiteral("Connected");
        break;
    default:
        statusText = QStringLiteral("UnknownStatus");
        break;
    }

    return QStringLiteral("Recovery failed for role=%1 deviceId=%2 status=%3 retries=%4/%5 intervalMs=%6 timeoutMs=%7")
        .arg(policy.roleName,
             deviceId,
             statusText)
        .arg(performedRetries)
        .arg(policy.maxRetries)
        .arg(policy.retryIntervalMs)
        .arg(policy.connectTimeoutMs);
}

} // namespace vc::model

#endif // LOCALIZATION_RECOVERY_POLICY_H
