#ifndef DEVICE_COMMAND_H
#define DEVICE_COMMAND_H

#include <QMetaType>
#include <QString>
#include <QUuid>
#include <QVariantMap>

#include <utility>

namespace vc::runtime {

enum class DeviceCommandKind {
    Unknown,
    Connect,
    Disconnect,
    CameraSingleShot,
    CameraApplyParams,
};

enum class DeviceCommandResultStatus {
    Accepted,
    Rejected,
    Succeeded,
    Failed,
};

enum class DeviceCommandResultCode {
    None,
    Busy,
    UnsupportedCommand,
    InvalidTarget,
    DeviceError,
    TimedOut,
    QueueFull,
};

inline QString deviceCommandKindToString(DeviceCommandKind kind)
{
    switch (kind) {
    case DeviceCommandKind::Connect:
        return QStringLiteral("Connect");
    case DeviceCommandKind::Disconnect:
        return QStringLiteral("Disconnect");
    case DeviceCommandKind::CameraSingleShot:
        return QStringLiteral("CameraSingleShot");
    case DeviceCommandKind::CameraApplyParams:
        return QStringLiteral("CameraApplyParams");
    case DeviceCommandKind::Unknown:
    default:
        return QStringLiteral("Unknown");
    }
}

inline QString deviceCommandResultStatusToString(DeviceCommandResultStatus status)
{
    switch (status) {
    case DeviceCommandResultStatus::Accepted:
        return QStringLiteral("Accepted");
    case DeviceCommandResultStatus::Rejected:
        return QStringLiteral("Rejected");
    case DeviceCommandResultStatus::Succeeded:
        return QStringLiteral("Succeeded");
    case DeviceCommandResultStatus::Failed:
    default:
        return QStringLiteral("Failed");
    }
}

inline QString deviceCommandResultCodeToString(DeviceCommandResultCode code)
{
    switch (code) {
    case DeviceCommandResultCode::None:
        return QStringLiteral("None");
    case DeviceCommandResultCode::Busy:
        return QStringLiteral("Busy");
    case DeviceCommandResultCode::UnsupportedCommand:
        return QStringLiteral("UnsupportedCommand");
    case DeviceCommandResultCode::InvalidTarget:
        return QStringLiteral("InvalidTarget");
    case DeviceCommandResultCode::DeviceError:
        return QStringLiteral("DeviceError");
    case DeviceCommandResultCode::TimedOut:
        return QStringLiteral("TimedOut");
    case DeviceCommandResultCode::QueueFull:
        return QStringLiteral("QueueFull");
    default:
        return QStringLiteral("UnknownCode");
    }
}

struct DeviceCommand {
    QString id;
    DeviceCommandKind kind{DeviceCommandKind::Unknown};
    QString targetDeviceId;
    int timeoutMs{3000};
    QVariantMap payload;

    static DeviceCommand create(DeviceCommandKind kind,
                                const QString &targetDeviceId,
                                int timeoutMs = 3000,
                                QVariantMap payload = {})
    {
        DeviceCommand command;
        command.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        command.kind = kind;
        command.targetDeviceId = targetDeviceId;
        command.timeoutMs = timeoutMs;
        command.payload = std::move(payload);
        return command;
    }
};

struct DeviceCommandResult {
    QString commandId;
    DeviceCommandKind kind{DeviceCommandKind::Unknown};
    QString targetDeviceId;
    DeviceCommandResultStatus status{DeviceCommandResultStatus::Failed};
    DeviceCommandResultCode code{DeviceCommandResultCode::None};
    QString message;
    QVariantMap payload;

    bool isAccepted() const
    {
        return status == DeviceCommandResultStatus::Accepted ||
               status == DeviceCommandResultStatus::Succeeded;
    }

    bool isTerminalSuccess() const
    {
        return status == DeviceCommandResultStatus::Succeeded;
    }

    static DeviceCommandResult accepted(const DeviceCommand &command,
                                        QString message = {})
    {
        return make(command,
                    DeviceCommandResultStatus::Accepted,
                    DeviceCommandResultCode::None,
                    std::move(message));
    }

    static DeviceCommandResult rejected(const DeviceCommand &command,
                                        DeviceCommandResultCode code,
                                        QString message)
    {
        return make(command,
                    DeviceCommandResultStatus::Rejected,
                    code,
                    std::move(message));
    }

    static DeviceCommandResult succeeded(const DeviceCommand &command,
                                         QString message = {},
                                         QVariantMap payload = {})
    {
        return make(command,
                    DeviceCommandResultStatus::Succeeded,
                    DeviceCommandResultCode::None,
                    std::move(message),
                    std::move(payload));
    }

    static DeviceCommandResult failed(const DeviceCommand &command,
                                      DeviceCommandResultCode code,
                                      QString message,
                                      QVariantMap payload = {})
    {
        return make(command,
                    DeviceCommandResultStatus::Failed,
                    code,
                    std::move(message),
                    std::move(payload));
    }

private:
    static DeviceCommandResult make(const DeviceCommand &command,
                                    DeviceCommandResultStatus status,
                                    DeviceCommandResultCode code,
                                    QString message,
                                    QVariantMap payload = {})
    {
        DeviceCommandResult result;
        result.commandId = command.id;
        result.kind = command.kind;
        result.targetDeviceId = command.targetDeviceId;
        result.status = status;
        result.code = code;
        result.message = std::move(message);
        result.payload = std::move(payload);
        return result;
    }
};

} // namespace vc::runtime

Q_DECLARE_METATYPE(vc::runtime::DeviceCommand)
Q_DECLARE_METATYPE(vc::runtime::DeviceCommandResult)

#endif // DEVICE_COMMAND_H
