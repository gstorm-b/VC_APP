#ifndef CAMERA_RUNNER_H
#define CAMERA_RUNNER_H

#include <QTimer>

#include "runtime/device_runner.h"
#include "runtime/device_command_queue.h"
#include "device/camera/camera_device.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  CameraRunner
//
//  (Replaces the legacy CameraWorker that previously lived in src/model.)
//
//  Commission mode
//  ───────────────
//    Widget calls requestConnect() / requestSingleShot() etc. from the GUI
//    thread.  These emit queued signals that the camera device processes on
//    its own HighPriority thread.  Results arrive back via grabFinished(),
//    parametersApplied() etc. (also queued → safe for GUI slots).
//
//  Runtime mode
//  ────────────
//    The camera stays on its own thread.  The task runtime thread triggers
//    grabs and awaits grabFinished() via QueuedConnection.  The runner
//    stays the mediator so no direct cross-thread calls are needed.
//
//  Camera requests route through the standard DeviceCommand model.
//  A31 adds queue and timeout behavior:
//    - Connect / Disconnect reject when another command is active.
//    - SingleShot enters FIFO queue while busy.
//    - ApplyParams keeps only the latest pending request.
// ─────────────────────────────────────────────────────────────────────────────

class CameraRunner : public DeviceRunner<vc::device::CameraDevice> {
    Q_OBJECT

public:
    explicit CameraRunner(vc::device::CameraDevice *camera,
                          QObject *parent = nullptr)
        : DeviceRunner(camera, parent)
    {
        qRegisterMetaType<DeviceCommand>("vc::runtime::DeviceCommand");
        qRegisterMetaType<DeviceCommandResult>("vc::runtime::DeviceCommandResult");
        qRegisterMetaType<vc::device::GrabResult>("vc::device::GrabResult");
        m_activeCommandTimer.setSingleShot(true);
        connect(&m_activeCommandTimer, &QTimer::timeout,
                this, &CameraRunner::onActiveCommandTimedOut);
    }

    // ── Commission / runtime actions (safe from any thread) ──────────────────
    void requestConnect()
    {
        submitCommand(DeviceCommand::create(DeviceCommandKind::Connect,
                                            m_device->id()));
    }

    void requestDisconnect()
    {
        submitCommand(DeviceCommand::create(DeviceCommandKind::Disconnect,
                                            m_device->id()));
    }

    void requestSingleShot()
    {
        submitCommand(DeviceCommand::create(DeviceCommandKind::CameraSingleShot,
                                            m_device->id()));
    }

    void requestApplyParams()
    {
        submitCommand(DeviceCommand::create(DeviceCommandKind::CameraApplyParams,
                                            m_device->id()));
    }

    DeviceCommandResult submitCommand(const DeviceCommand &command) override
    {
        if (command.targetDeviceId != m_device->id()) {
            DeviceCommandResult result = DeviceCommandResult::rejected(
                command,
                DeviceCommandResultCode::InvalidTarget,
                QStringLiteral("Camera command target does not match this runner."));
            finishRejectedCommand(result);
            return result;
        }

        if (!isSupportedCommand(command.kind)) {
            DeviceCommandResult result = DeviceCommandResult::rejected(
                command,
                DeviceCommandResultCode::UnsupportedCommand,
                QStringLiteral("Unsupported camera command."));
            finishRejectedCommand(result);
            return result;
        }

        const DeviceCommandQueuePolicy policy = queuePolicyFor(command.kind);
        const DeviceCommandEnqueueDecision decision =
            m_commandQueue.enqueue(command, policy, hasActiveCommand());
        if (!decision.accepted) {
            finishRejectedCommand(decision.rejection);
            return decision.rejection;
        }

        if (decision.shouldRunNow) {
            runCommand(command);
            return DeviceCommandResult::accepted(command);
        }

        const QString queueMessage = decision.replacedPending
                                         ? QStringLiteral("Camera command queued (replaced pending same-kind request).")
                                         : QStringLiteral("Camera command queued.");
        LOG_USER_INFO << "Camera command queued."
                      << "id=" << command.id
                      << "kind=" << deviceCommandKindToString(command.kind)
                      << "target=" << command.targetDeviceId
                      << "pending=" << decision.pendingCount;
        return DeviceCommandResult::accepted(command, queueMessage);
    }

signals:
    // ── Results forwarded from camera thread ──────────────────────────────────
    void grabFinished(vc::device::GrabResult result);
    void parametersApplied(bool ok);

    // ── Internal queued triggers (→ camera thread) ────────────────────────────
    void sig_connect();
    void sig_disconnect();
    void sig_singleShot();
    void sig_applyParams();

protected:
    void wireSignals() override {
        using Cam = vc::device::CameraDevice;
        using Run = CameraRunner;

        connect(this,     &Run::sig_connect,
                m_device, &Cam::deviceConnect,         Qt::QueuedConnection);
        connect(this,     &Run::sig_disconnect,
                m_device, &Cam::deviceDisconnect,      Qt::QueuedConnection);
        connect(this,     &Run::sig_singleShot,
                m_device, &Cam::grabSingleShot,        Qt::QueuedConnection);
        connect(this,     &Run::sig_applyParams,
                m_device, &Cam::applyParametersChange, Qt::QueuedConnection);

        connect(m_device, &Cam::connectStatusChanged,
                this,     &Run::onConnectStatusChanged, Qt::QueuedConnection);
        connect(m_device, &Cam::connectionFailed,
                this,     &Run::onConnectionFailed,     Qt::QueuedConnection);
        connect(m_device, &Cam::grabFinished,
                this,     &Run::onGrabFinished,         Qt::QueuedConnection);
        connect(m_device, &Cam::parametersApplied,
                this,     &Run::onParametersApplied,    Qt::QueuedConnection);
        connect(m_device, &Cam::errorOccurred,
                this,     &Run::errorOccurred,          Qt::QueuedConnection);
    }

    void unwireSignals() override {
        disconnect(this,     nullptr, m_device, nullptr);
        disconnect(m_device, nullptr, this,     nullptr);
    }

private slots:
    void onConnectStatusChanged(vc::device::ConnectStatus status) {
        if (!hasActiveCommand()) {
            emit connectStatusChanged(status);
            return;
        }

        if (m_activeCommand.kind == DeviceCommandKind::Connect) {
            if (status == vc::device::ConnectStatus::Connected) {
                finishActiveCommand(DeviceCommandResult::succeeded(
                    m_activeCommand,
                    QStringLiteral("Camera connected.")));
            } else if (status == vc::device::ConnectStatus::ConnectFailed ||
                       status == vc::device::ConnectStatus::LostConnected) {
                finishActiveCommand(DeviceCommandResult::failed(
                    m_activeCommand,
                    DeviceCommandResultCode::DeviceError,
                    QStringLiteral("Camera connection failed.")));
            }
        } else if (m_activeCommand.kind == DeviceCommandKind::Disconnect) {
            if (status == vc::device::ConnectStatus::Disconnected ||
                status == vc::device::ConnectStatus::NoConnection) {
                finishActiveCommand(DeviceCommandResult::succeeded(
                    m_activeCommand,
                    QStringLiteral("Camera disconnected.")));
            }
        }

        emit connectStatusChanged(status);
    }

    void onConnectionFailed(const QString &msg) {
        if (hasActiveCommand()) {
            finishActiveCommand(DeviceCommandResult::failed(
                m_activeCommand,
                DeviceCommandResultCode::DeviceError,
                msg));
        }
        emit errorOccurred(msg);
    }

    void onGrabFinished(vc::device::GrabResult result) {
        QVariantMap payload;
        payload.insert(QStringLiteral("message"), result.msg);
        if (!hasActiveCommand()) {
            emit grabFinished(result);
            return;
        }

        if (m_activeCommand.kind == DeviceCommandKind::CameraSingleShot &&
            result.isGrabSuccess) {
            finishActiveCommand(DeviceCommandResult::succeeded(
                m_activeCommand,
                result.msg,
                payload));
        } else if (m_activeCommand.kind == DeviceCommandKind::CameraSingleShot) {
            finishActiveCommand(DeviceCommandResult::failed(
                m_activeCommand,
                DeviceCommandResultCode::DeviceError,
                result.msg,
                payload));
        }
        emit grabFinished(result);
    }

    void onParametersApplied(bool ok) {
        if (!hasActiveCommand()) {
            emit parametersApplied(ok);
            return;
        }

        if (m_activeCommand.kind == DeviceCommandKind::CameraApplyParams && ok) {
            finishActiveCommand(DeviceCommandResult::succeeded(
                m_activeCommand,
                QStringLiteral("Camera parameters applied.")));
        } else if (m_activeCommand.kind == DeviceCommandKind::CameraApplyParams) {
            finishActiveCommand(DeviceCommandResult::failed(
                m_activeCommand,
                DeviceCommandResultCode::DeviceError,
                QStringLiteral("Camera parameters apply failed.")));
        }
        emit parametersApplied(ok);
    }

    void onActiveCommandTimedOut()
    {
        if (!hasActiveCommand()) {
            return;
        }

        const DeviceCommand timedOut = m_activeCommand;
        finishActiveCommand(DeviceCommandResult::failed(
            timedOut,
            DeviceCommandResultCode::TimedOut,
            QStringLiteral("Camera command timed out.")));
    }

private:
    using TriggerSignal = void (CameraRunner::*)();

    static bool isSupportedCommand(DeviceCommandKind kind)
    {
        switch (kind) {
        case DeviceCommandKind::Connect:
        case DeviceCommandKind::Disconnect:
        case DeviceCommandKind::CameraSingleShot:
        case DeviceCommandKind::CameraApplyParams:
            return true;
        case DeviceCommandKind::Unknown:
        default:
            return false;
        }
    }

    static DeviceCommandQueuePolicy queuePolicyFor(DeviceCommandKind kind)
    {
        switch (kind) {
        case DeviceCommandKind::Connect:
        case DeviceCommandKind::Disconnect:
            return DeviceCommandQueuePolicy::RejectWhenBusy;
        case DeviceCommandKind::CameraSingleShot:
            return DeviceCommandQueuePolicy::QueueWhenBusy;
        case DeviceCommandKind::CameraApplyParams:
            return DeviceCommandQueuePolicy::ReplacePendingSameKind;
        case DeviceCommandKind::Unknown:
        default:
            return DeviceCommandQueuePolicy::RejectWhenBusy;
        }
    }

    bool hasActiveCommand() const
    {
        return !m_activeCommand.id.isEmpty();
    }

    void runCommand(const DeviceCommand &command)
    {
        const TriggerSignal trigger = triggerFor(command.kind);
        if (trigger == nullptr) {
            DeviceCommandResult result = DeviceCommandResult::rejected(
                command,
                DeviceCommandResultCode::UnsupportedCommand,
                QStringLiteral("Camera command trigger is not available."));
            finishRejectedCommand(result);
            return;
        }

        m_activeCommand = command;
        m_activeCommandTimer.start(activeTimeoutMs(command));
        LOG_USER_INFO << "Camera command started."
                      << "id=" << command.id
                      << "kind=" << deviceCommandKindToString(command.kind)
                      << "target=" << command.targetDeviceId
                      << "timeoutMs=" << activeTimeoutMs(command)
                      << "pending=" << m_commandQueue.pendingCount();
        (this->*trigger)();
    }

    TriggerSignal triggerFor(DeviceCommandKind kind) const
    {
        switch (kind) {
        case DeviceCommandKind::Connect:
            return &CameraRunner::sig_connect;
        case DeviceCommandKind::Disconnect:
            return &CameraRunner::sig_disconnect;
        case DeviceCommandKind::CameraSingleShot:
            return &CameraRunner::sig_singleShot;
        case DeviceCommandKind::CameraApplyParams:
            return &CameraRunner::sig_applyParams;
        case DeviceCommandKind::Unknown:
        default:
            return nullptr;
        }
    }

    static int activeTimeoutMs(const DeviceCommand &command)
    {
        return command.timeoutMs > 0 ? command.timeoutMs : 3000;
    }

    void finishRejectedCommand(const DeviceCommandResult &result)
    {
        LOG_USER_INFO << "Camera command rejected."
                      << "id=" << result.commandId
                      << "kind=" << deviceCommandKindToString(result.kind)
                      << "target=" << result.targetDeviceId
                      << "status=" << deviceCommandResultStatusToString(result.status)
                      << "code=" << deviceCommandResultCodeToString(result.code)
                      << "msg=" << result.message;
        emit commandFinished(result);
    }

    void finishActiveCommand(const DeviceCommandResult &result)
    {
        m_activeCommandTimer.stop();
        LOG_USER_INFO << "Camera command finished."
                      << "id=" << result.commandId
                      << "kind=" << deviceCommandKindToString(result.kind)
                      << "target=" << result.targetDeviceId
                      << "status=" << deviceCommandResultStatusToString(result.status)
                      << "code=" << deviceCommandResultCodeToString(result.code)
                      << "msg=" << result.message;
        emit commandFinished(result);
        clearActiveCommand();
        dispatchNextCommand();
    }

    void clearActiveCommand()
    {
        m_activeCommand = DeviceCommand();
        m_activeCommandTimer.stop();
    }

    void dispatchNextCommand()
    {
        if (hasActiveCommand() || !m_commandQueue.hasPending()) {
            return;
        }

        runCommand(m_commandQueue.takeNext());
    }

    DeviceCommandQueue m_commandQueue;
    QTimer m_activeCommandTimer;
    DeviceCommand m_activeCommand;
};

} // namespace vc::runtime

#endif // CAMERA_RUNNER_H
