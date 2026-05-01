#ifndef MC_DEVICE_WORKER_H
#define MC_DEVICE_WORKER_H

#include <QObject>
#include <QThread>
#include "device/plc/mc_protocol_device.h"

namespace vc::runtime {

class McDeviceWorker : public QObject {
    Q_OBJECT
public:
    explicit McDeviceWorker(device::McProtocolDevice* device,
                          QObject *parent = nullptr)
        : QObject(parent), m_device(device) {


        m_worker = new QThread();
        m_worker->start(QThread::HighPriority);
    }

    ~McDeviceWorker() {


    }

    void moveToWorker() {
        m_device->moveToThread(m_worker);

        connect(this, &McDeviceWorker::trigger_connect,
                m_device, &vc::device::McProtocolDevice::deviceConnect, Qt::QueuedConnection);

        connect(this, &McDeviceWorker::trigger_disconnect,
                m_device, &vc::device::McProtocolDevice::deviceDisconnect, Qt::QueuedConnection);

        // connect(this, &McDeviceWorker::trigger_sigleShot,
        //         m_device, &vc::device::McProtocolDevice::grabSingleShot, Qt::QueuedConnection);


        // connect(m_device, &vc::device::CameraDevice::connectStatusChanged,
        //         this, &CameraWorker::onConnectionStatusChanged, Qt::QueuedConnection);

        // connect(m_device, &vc::device::CameraDevice::connectionFailed,
        //         this, &CameraWorker::onConnectionFailed, Qt::QueuedConnection);

        // connect(m_device, &vc::device::CameraDevice::parametersApplied,
        //         this, &CameraWorker::onParameterChanged, Qt::QueuedConnection);

        // connect(m_device, &vc::device::CameraDevice::grabFinished,
        //         this, &CameraWorker::onCameraGrabFinished, Qt::QueuedConnection);

        // connect(m_worker, &QThread::finished, m_device, &device::McProtocolDevice::deviceTerminate);
    }

    void moveOut(QThread *thread) {
        disconnect(this, &McDeviceWorker::trigger_connect,
                   m_device, &vc::device::McProtocolDevice::deviceConnect);

        disconnect(this, &McDeviceWorker::trigger_disconnect,
                   m_device, &vc::device::McProtocolDevice::deviceDisconnect);

        // disconnect(this, &McDeviceWorker::trigger_sigleShot,
        //            m_device, &vc::device::McProtocolDevice::grabSingleShot);

        // disconnect(m_device, &vc::device::CameraDevice::connectStatusChanged,
        //            this, &CameraWorker::onConnectionStatusChanged);

        // disconnect(m_device, &vc::device::CameraDevice::connectionFailed,
        //            this, &CameraWorker::onConnectionFailed);

        // disconnect(m_device, &vc::device::CameraDevice::parametersApplied,
        //            this, &CameraWorker::onParameterChanged);

        // disconnect(m_device, &vc::device::CameraDevice::grabFinished,
        //            this, &CameraWorker::onCameraGrabFinished);

        // disconnect(m_worker, &QThread::finished, m_device, &device::McProtocolDevice::deviceTerminate);
        m_device->moveToThread(thread);
    }

    void connectMcDevice() {
        if (m_device->isDeviceConnected()) {
            return;
        }
        emit trigger_connect();
    }

    void disconnectMcDevice() {
        if (!m_device->isDeviceConnected()) {
            return;
        }
        emit trigger_disconnect();
    }


signals:
    void trigger_connect();
    void trigger_disconnect();
    // void trigger_apply_change();
    // void trigger_sigleShot();

private:
    QMutex m_mutex;
    device::McProtocolDevice* m_device;
    QThread *m_worker{nullptr};
    bool m_busy{false};
};


} // vc::runtime


#endif // MC_DEVICE_WORKER_H
