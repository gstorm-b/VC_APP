#ifndef MC_DEVICE_RUNNER_H
#define MC_DEVICE_RUNNER_H

#include "runtime/device_runner.h"
#include "device/plc/mc_protocol_device.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  McDeviceRunner
//
//  (Replaces the legacy McDeviceWorker that previously lived in src/model.)
//
//  Commission mode
//  ───────────────
//    requestConnect() / requestDisconnect() from GUI thread → queued to PLC
//    thread.  Connection status comes back via connectStatusChanged().
//
//  Runtime mode
//  ────────────
//    The McProtocolDevice has an internal QTimer-based polling loop.  Once
//    attached and connected, the polling runs autonomously on its thread.
//    The task runtime thread listens to pollingUpdate() and deviceMChanged()
//    / deviceDChanged() to react to PLC state changes.
// ─────────────────────────────────────────────────────────────────────────────

class McDeviceRunner : public DeviceRunner<vc::device::McProtocolDevice> {
    Q_OBJECT

public:
    explicit McDeviceRunner(vc::device::McProtocolDevice *plc,
                            QObject *parent = nullptr)
        : DeviceRunner(plc, parent) {}

    // ── Commission actions (safe from any thread) ─────────────────────────────
    void requestConnect()    { if (!m_busy) { m_busy = true; emit sig_connect();    } }
    void requestDisconnect() { if (!m_busy) { m_busy = true; emit sig_disconnect(); } }

signals:
    // ── Forwarded from PLC thread ─────────────────────────────────────────────
    void pollingUpdate(vc::device::McDeviceMap map);
    void requestFinished(vc::device::McResult result);
    void deviceMChanged(int number, quint8 lastState, quint8 newState);
    void deviceDChanged(int number, qint16 lastVal, qint16 newVal);

    // ── Internal queued triggers ──────────────────────────────────────────────
    void sig_connect();
    void sig_disconnect();

protected:
    void wireSignals() override {
        using Plc = vc::device::McProtocolDevice;
        using Run = McDeviceRunner;

        connect(this,     &Run::sig_connect,
                m_device, &Plc::deviceConnect,    Qt::QueuedConnection);
        connect(this,     &Run::sig_disconnect,
                m_device, &Plc::deviceDisconnect, Qt::QueuedConnection);

        connect(m_device, &Plc::connectStatusChanged,
                this,     &Run::onConnectStatusChanged, Qt::QueuedConnection);
        connect(m_device, &Plc::connectionFailed,
                this,     &Run::onConnectionFailed,     Qt::QueuedConnection);
        connect(m_device, &Plc::pollingUpdate,
                this,     &Run::pollingUpdate,           Qt::QueuedConnection);
        connect(m_device, &Plc::requestFinished,
                this,     &Run::requestFinished,         Qt::QueuedConnection);
        connect(m_device, &Plc::deviceMChanged,
                this,     &Run::deviceMChanged,          Qt::QueuedConnection);
        connect(m_device, &Plc::deviceDChanged,
                this,     &Run::deviceDChanged,          Qt::QueuedConnection);
        connect(m_device, &Plc::errorOccurred,
                this,     &Run::errorOccurred,           Qt::QueuedConnection);
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

#endif // MC_DEVICE_RUNNER_H
