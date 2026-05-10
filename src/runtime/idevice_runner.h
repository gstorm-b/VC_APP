#ifndef IDEVICE_RUNNER_H
#define IDEVICE_RUNNER_H

#include <QObject>
#include <QThread>
#include "device/idevice.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  IDeviceRunner
//
//  Abstract interface for per-device thread controllers.
//  Concrete implementations (CameraRunner, McDeviceRunner) inherit from the
//  DeviceRunner<T> template base which provides the boilerplate.
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
    virtual QThread             *workerThread() const = 0;
    virtual vc::device::IDevice *device()       const = 0;

    bool isAttached() const { return m_attached; }
    bool isRunning()  const { return workerThread() && workerThread()->isRunning(); }

signals:
    // Forwarded from the concrete device (cross-thread safe)
    void connectStatusChanged(vc::device::ConnectStatus status);
    void errorOccurred(const QString &msg);

protected:
    bool m_attached{false};
};

} // namespace vc::runtime

#endif // IDEVICE_RUNNER_H
