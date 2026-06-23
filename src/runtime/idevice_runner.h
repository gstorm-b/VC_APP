#ifndef IDEVICE_RUNNER_H
#define IDEVICE_RUNNER_H

#include <QObject>
#include <QThread>
#include <QEventLoop>
#include <QTimer>
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

    // Synchronously close the device connection on its own worker thread.
    //
    // Must be called BEFORE detach()/stop() during a phase teardown: the device
    // owns thread-affined transport objects (e.g. a QTcpSocket created on the
    // worker thread). Moving the device across threads or stopping the worker
    // while the link is still open leaks the connection and can crash on
    // re-entry. Triggering deviceDisconnect() here lets the device close its
    // transport on the correct thread.
    //
    // No-op when the device is not connected. Bounded by timeoutMs so a
    // misbehaving device cannot stall teardown. Requires the runner to still be
    // attached with its worker thread running (the normal pre-detach state).
    void disconnectAndWait(int timeoutMs = 3000) {
        vc::device::IDevice *dev = device();
        if (!dev) return;

        const vc::device::ConnectStatus status = dev->connectStatus();
        if (status != vc::device::ConnectStatus::Connected &&
            status != vc::device::ConnectStatus::Connecting &&
            status != vc::device::ConnectStatus::LostConnected) {
            return;   // nothing to tear down
        }

        QEventLoop loop;
        QTimer guard;
        guard.setSingleShot(true);

        const QMetaObject::Connection statusConn = connect(
            this, &IDeviceRunner::connectStatusChanged, &loop,
            [&loop](vc::device::ConnectStatus s) {
                if (s == vc::device::ConnectStatus::Disconnected ||
                    s == vc::device::ConnectStatus::NoConnection) {
                    loop.quit();
                }
            });
        connect(&guard, &QTimer::timeout, &loop, &QEventLoop::quit);
        connect(this, &IDeviceRunner::requestDeviceDisconnect,
                dev, &vc::device::IDevice::deviceDisconnect, Qt::SingleShotConnection);

        guard.start(timeoutMs);
        emit requestDeviceDisconnect();
        loop.exec();
        disconnect(statusConn);
    }

signals:
    // Forwarded from the concrete device (cross-thread safe)
    void requestDetach(QThread *dest);
    void requestDeviceDisconnect();
    // void detachFinished();
    void connectStatusChanged(vc::device::ConnectStatus status);
    void errorOccurred(const QString &msg);
    void commandFinished(vc::runtime::DeviceCommandResult result);

protected:
    bool m_attached{false};
};

} // namespace vc::runtime

#endif // IDEVICE_RUNNER_H
