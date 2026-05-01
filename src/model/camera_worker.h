#ifndef CAMERA_WORKER_H
#define CAMERA_WORKER_H

#include <QObject>
#include <QThread>
#include "device/camera/camera_device.h"

namespace vc::runtime {

class CameraWorker : public QObject {
    Q_OBJECT
public:
    explicit CameraWorker(device::CameraDevice* device,
                          QObject *parent = nullptr)
        : QObject(parent), m_device(device) {


        m_worker = new QThread();
        m_worker->start(QThread::HighPriority);
    }

    ~CameraWorker() {
        m_worker->exit();
        m_worker->deleteLater();
    }

    QThread *workerThread() {
        return m_worker;
    }

    void moveToWorker() {
        m_device->moveToThread(m_worker);

        connect(this, &CameraWorker::trigger_connect,
                m_device, &vc::device::CameraDevice::deviceConnect, Qt::QueuedConnection);

        connect(this, &CameraWorker::trigger_disconnect,
                m_device, &vc::device::CameraDevice::deviceDisconnect, Qt::QueuedConnection);

        connect(this, &CameraWorker::trigger_apply_change,
                m_device, &vc::device::CameraDevice::applyParametersChange, Qt::QueuedConnection);

        connect(this, &CameraWorker::trigger_sigleShot,
                m_device, &vc::device::CameraDevice::grabSingleShot, Qt::QueuedConnection);


        connect(m_device, &vc::device::CameraDevice::connectStatusChanged,
                this, &CameraWorker::onConnectionStatusChanged, Qt::QueuedConnection);

        connect(m_device, &vc::device::CameraDevice::connectionFailed,
                this, &CameraWorker::onConnectionFailed, Qt::QueuedConnection);

        connect(m_device, &vc::device::CameraDevice::parametersApplied,
                this, &CameraWorker::onParameterChanged, Qt::QueuedConnection);

        connect(m_device, &vc::device::CameraDevice::grabFinished,
                this, &CameraWorker::onCameraGrabFinished, Qt::QueuedConnection);
    }

    void moveOut(QThread *thread) {
        disconnect(this, &CameraWorker::trigger_connect,
                m_device, &vc::device::CameraDevice::deviceConnect);

        disconnect(this, &CameraWorker::trigger_disconnect,
                m_device, &vc::device::CameraDevice::deviceDisconnect);

        disconnect(this, &CameraWorker::trigger_apply_change,
                   m_device, &vc::device::CameraDevice::applyParametersChange);

        disconnect(this, &CameraWorker::trigger_sigleShot,
                m_device, &vc::device::CameraDevice::grabSingleShot);


        disconnect(m_device, &vc::device::CameraDevice::connectStatusChanged,
                this, &CameraWorker::onConnectionStatusChanged);

        disconnect(m_device, &vc::device::CameraDevice::connectionFailed,
                this, &CameraWorker::onConnectionFailed);

        disconnect(m_device, &vc::device::CameraDevice::parametersApplied,
                   this, &CameraWorker::onParameterChanged);

        disconnect(m_device, &vc::device::CameraDevice::grabFinished,
                this, &CameraWorker::onCameraGrabFinished);

        m_device->moveToThread(thread);
    }

    void connectCamera() {
        if (m_busy) return;
        m_busy = true;
        emit trigger_connect();
    }

    void disconnectCamera() {
        if (m_busy) return;
        m_busy = true;
        emit trigger_disconnect();
    }

    void applyChangeParameters() {
        if (m_busy) return;
        m_busy = true;
        emit trigger_apply_change();
    }

    void singleShot() {
        if (m_busy) return;
        m_busy = true;
        emit trigger_sigleShot();
    }

private slots:
    void stop_worker(QObject *obj) {

    }

    void onConnectionStatusChanged(bool) {
        m_busy = false;
    }

    void onConnectionFailed(QString) {
        m_busy = false;
    }

    void onParameterChanged(bool isOk) {
        LOG_USER_INFO << tr("Camera parameter change: %1").arg(m_device->lastMsg());
        m_busy = false;
    }

    void onCameraGrabFinished(vc::device::GrabResult result) {
        Q_UNUSED(result);
        m_busy = false;
    }

signals:
    void trigger_connect();
    void trigger_disconnect();
    void trigger_apply_change();
    void trigger_sigleShot();

private:
    device::CameraDevice* m_device;
    QThread *m_worker{nullptr};
    bool m_busy{false};
};

}

#endif // CAMERA_WORKER_H
