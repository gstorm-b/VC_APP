#ifndef CAMERA_RUNNER_H
#define CAMERA_RUNNER_H

#include "runtime/device_runner.h"
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
//  m_busy prevents overlapping requests within a single runner instance.
// ─────────────────────────────────────────────────────────────────────────────

class CameraRunner : public DeviceRunner<vc::device::CameraDevice> {
    Q_OBJECT

public:
    explicit CameraRunner(vc::device::CameraDevice *camera,
                          QObject *parent = nullptr)
        : DeviceRunner(camera, parent) {}

    // ── Commission / runtime actions (safe from any thread) ──────────────────
    void requestConnect()     { if (!m_busy) { m_busy = true; emit sig_connect();     } }
    void requestDisconnect()  { if (!m_busy) { m_busy = true; emit sig_disconnect();  } }
    void requestSingleShot()  { if (!m_busy) { m_busy = true; emit sig_singleShot();  } }
    void requestApplyParams() { if (!m_busy) { m_busy = true; emit sig_applyParams(); } }

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
        m_busy = false;
        emit connectStatusChanged(status);
    }

    void onConnectionFailed(const QString &msg) {
        m_busy = false;
        emit errorOccurred(msg);
    }

    void onGrabFinished(vc::device::GrabResult result) {
        m_busy = false;
        emit grabFinished(result);
    }

    void onParametersApplied(bool ok) {
        m_busy = false;
        emit parametersApplied(ok);
    }

private:
    bool m_busy{false};
};

} // namespace vc::runtime

#endif // CAMERA_RUNNER_H
