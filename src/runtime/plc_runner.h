#ifndef PLC_RUNNER_H
#define PLC_RUNNER_H

#include "runtime/device_runner.h"
#include "device/plc/plc_device.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  PlcRunner
//
//  Family-level runner for the PLC device family. Holds a PlcDevice* (the
//  abstract base) so the runner is sub-type-agnostic. Vendor-specific
//  consumers reach the concrete device via
//  `qobject_cast<McProtocolDevice *>(runner->typedDevice())` and connect to
//  the device's vendor signals directly (cross-thread queued).
//
//  Mirror of CameraRunner / VisionOutputRunner: one flat runner per family,
//  no per-vendor subclass.
//
//  Commission mode
//  ───────────────
//    requestConnect() / requestDisconnect() from GUI thread → queued to PLC
//    thread. Connection status comes back via connectStatusChanged().
//
//  Runtime mode
//  ────────────
//    The concrete PLC device runs its own polling loop on its thread. The
//    task runtime thread listens to pollingUpdate() forwarded here.
// ─────────────────────────────────────────────────────────────────────────────

class PlcRunner : public DeviceRunner<vc::device::PlcDevice> {
    Q_OBJECT

public:
    explicit PlcRunner(vc::device::PlcDevice *plc,
                       QObject *parent = nullptr)
        : DeviceRunner(plc, parent) {}

    // ── Commission actions (safe from any thread) ─────────────────────────────
    void requestConnect()    { if (!m_busy) { m_busy = true; emit sig_connect();    } }
    void requestDisconnect() { if (!m_busy) { m_busy = true; emit sig_disconnect(); } }

signals:
    // ── Family-level signals forwarded from PLC thread ────────────────────────
    void pollingUpdate(std::shared_ptr<vc::device::PlcValueMap> map);
    void valueChanged(QMap<QString, QVariant> values);

    // ── Internal queued triggers ──────────────────────────────────────────────
    void sig_connect();
    void sig_disconnect();

protected:
    void wireSignals() override {
        using Plc = vc::device::PlcDevice;
        using Run = PlcRunner;

        connect(this,     &Run::sig_connect,
                m_device, &Plc::deviceConnect,    Qt::QueuedConnection);
        connect(this,     &Run::sig_disconnect,
                m_device, &Plc::deviceDisconnect, Qt::QueuedConnection);

        connect(m_device, &Plc::connectStatusChanged,
                this,     &Run::onConnectStatusChanged, Qt::QueuedConnection);
        connect(m_device, &Plc::connectionFailed,
                this,     &Run::onConnectionFailed,     Qt::QueuedConnection);
        connect(m_device, &Plc::pollingUpdate,
                this,     &Run::pollingUpdate,          Qt::QueuedConnection);
        connect(m_device, &Plc::valueChanged,
                this,     &Run::valueChanged,           Qt::QueuedConnection);
        connect(m_device, &Plc::errorOccurred,
                this,     &Run::errorOccurred,          Qt::QueuedConnection);
    }

    void unwireSignals() override {
        disconnect(this,     nullptr, m_device, nullptr);
        disconnect(m_device, nullptr, this,     nullptr);
    }

private slots:
    void onConnectStatusChanged(vc::device::ConnectStatus status) {
        m_busy = false;
        emit connectStatusChanged(status);
    }

    void onConnectionFailed(const QString &msg) {
        m_busy = false;
        emit errorOccurred(msg);
    }

private:
    bool m_busy{false};
};

} // namespace vc::runtime

#endif // PLC_RUNNER_H
