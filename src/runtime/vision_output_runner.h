#ifndef VISION_OUTPUT_RUNNER_H
#define VISION_OUTPUT_RUNNER_H

#include "runtime/device_runner.h"
#include "device/output_device/vision_output_device.h"


namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  VisionOutputRunner
//
// ─────────────────────────────────────────────────────────────────────────────

class VisionOutputRunner : public DeviceRunner<vc::device::VisionOutputDevice> {
    Q_OBJECT

public:
    explicit VisionOutputRunner(vc::device::VisionOutputDevice *vision_output,
                            QObject *parent = nullptr)
        : DeviceRunner(vision_output, parent) {}

    // ── Commission actions (safe from any thread) ─────────────────────────────
    void requestConnect()    { if (!m_busy) { m_busy = true; emit sig_connect();    } }
    void requestDisconnect() { if (!m_busy) { m_busy = true; emit sig_disconnect(); } }

signals:

    // ── Internal queued triggers ──────────────────────────────────────────────
    void sig_connect();
    void sig_disconnect();

protected:
    void wireSignals() override {
        using Plc = vc::device::VisionOutputDevice;
        using Run = VisionOutputRunner;

        connect(this,     &Run::sig_connect,
                m_device, &Plc::deviceConnect,    Qt::QueuedConnection);
        connect(this,     &Run::sig_disconnect,
                m_device, &Plc::deviceDisconnect, Qt::QueuedConnection);

        connect(m_device, &Plc::connectStatusChanged,
                this,     &Run::onConnectStatusChanged, Qt::QueuedConnection);
        connect(m_device, &Plc::connectionFailed,
                this,     &Run::onConnectionFailed,     Qt::QueuedConnection);
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


#endif // VISION_OUTPUT_RUNNER_H
