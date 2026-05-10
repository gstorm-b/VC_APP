#ifndef DEVICE_RUNNER_H
#define DEVICE_RUNNER_H

#include "runtime/idevice_runner.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  DeviceRunner<TDevice>
//
//  CRTP template base — eliminates the copy-paste that existed between the
//  old CameraWorker and McDeviceWorker.  Concrete subclasses only need to
//  implement wireSignals() / unwireSignals() with device-specific connections.
//
//  Thread ownership
//  ────────────────
//  m_thread is parented to this runner (QThread(this)) and is automatically
//  cleaned up.  The device pointer is NOT owned here; it is a raw pointer
//  into the shared_ptr held by DeviceManager.
//
//  attach() / detach() guard against double-calling via m_attached.
// ─────────────────────────────────────────────────────────────────────────────

template<typename TDevice>
class DeviceRunner : public IDeviceRunner {

public:
    explicit DeviceRunner(TDevice *device, QObject *parent = nullptr)
        : IDeviceRunner(parent)
        , m_device(device)
    {
        Q_ASSERT(device != nullptr);
        m_thread = new QThread(this);
    }

    ~DeviceRunner() override {
        stop();
    }

    // ── IDeviceRunner ─────────────────────────────────────────────────────────

    void start() override {
        if (!m_thread->isRunning())
            m_thread->start(QThread::HighPriority);
    }

    void stop() override {
        if (m_thread->isRunning()) {
            m_thread->quit();
            m_thread->wait(3000);
        }
    }

    void attach() override {
        if (m_attached) return;
        m_device->moveToThread(m_thread);
        wireSignals();
        m_attached = true;
    }

    void detach(QThread *dest = nullptr) override {
        if (!m_attached) return;
        unwireSignals();
        m_device->moveToThread(dest ? dest : QThread::currentThread());
        m_attached = false;
    }

    QThread             *workerThread() const override { return m_thread;  }
    vc::device::IDevice *device()       const override { return m_device; }

    // Type-safe accessor for concrete subclasses
    TDevice *typedDevice() const { return m_device; }

protected:
    // ── Subclass implements ──────────────────────────────────────────────────
    // Connect device-specific signals/slots using Qt::QueuedConnection.
    virtual void wireSignals()   = 0;
    virtual void unwireSignals() = 0;

    TDevice *m_device{nullptr};
    QThread *m_thread{nullptr};
};

} // namespace vc::runtime

#endif // DEVICE_RUNNER_H
