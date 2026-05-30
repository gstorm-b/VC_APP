#ifndef IDEVICE_RUNNER_H
#define IDEVICE_RUNNER_H

#include <QObject>
#include <QThread>
#include <QEventLoop>
#include "device/idevice.h"
#include "runtime/device_command.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  IDeviceRunner
//
//  Abstract interface for per-device thread controllers.
//  Concrete implementations (CameraRunner, PlcRunner, VisionOutputRunner)
//  inherit from the DeviceRunner<T> template base which provides the
//  boilerplate.
//
//  Lifecycle
//  ─────────
//    start()  → starts the internal QThread
//    attach() → device.moveToThread(worker) + wireSignals()
//    detach() → unwireSignals() + device.moveToThread(dest)
//    stop()   → thread.quit() + wait(3 s)
//
//  All signals forwarded from the device are emitted as Qt::QueuedConnection
//  and can be safely received on the GUI thread.
// ─────────────────────────────────────────────────────────────────────────────

class IDeviceRunner : public QObject {
    Q_OBJECT

public:
    explicit IDeviceRunner(QObject *parent = nullptr) : QObject(parent) {}
    ~IDeviceRunner() override = default;

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    virtual void start()                         = 0;
    virtual void stop()                          = 0;
    virtual void attach()                        = 0;
    virtual void detach(QThread *dest = nullptr) = 0;

    // ── State ─────────────────────────────────────────────────────────────────
    virtual QThread             *workerThread()   const = 0;
    virtual vc::device::IDevice *device()         const = 0;

    virtual DeviceCommandResult submitCommand(const DeviceCommand &command)
    {
        auto result = DeviceCommandResult::rejected(
            command,
            DeviceCommandResultCode::UnsupportedCommand,
            QStringLiteral("Runner does not support standard commands yet."));
        emit commandFinished(result);
        return result;
    }

    bool isAttached() const { return m_attached; }
    bool isRunning()  const { return workerThread() && workerThread()->isRunning(); }

    void detachFromOutsideThread(QThread *dest = nullptr) {
        QEventLoop loop;
        connect(this, &IDeviceRunner::requestDetach,
                device(), &device::IDevice::deviceDetachThread, Qt::SingleShotConnection);
        connect(device(), &device::IDevice::deviceThreadDetached,
                &loop, &QEventLoop::quit, Qt::SingleShotConnection);
        emit requestDetach(dest);
        loop.exec();
    }

signals:
    // Forwarded from the concrete device (cross-thread safe)
    void requestDetach(QThread *dest);
    // void detachFinished();
    void connectStatusChanged(vc::device::ConnectStatus status);
    void errorOccurred(const QString &msg);
    void commandFinished(vc::runtime::DeviceCommandResult result);

protected:
    bool m_attached{false};
};

} // namespace vc::runtime

#endif // IDEVICE_RUNNER_H
