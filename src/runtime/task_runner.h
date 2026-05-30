#ifndef TASK_RUNNER_H
#define TASK_RUNNER_H

#include <QObject>
#include <QMap>
#include <QThread>
#include <memory>

#include "runtime/idevice_runner.h"
#include "device/device_manager.h"

namespace vc::runtime {

// ─────────────────────────────────────────────────────────────────────────────
//  TaskRunner
//
//  Owned by ITask.  Central manager for all device threads belonging to one
//  task.  Drives the three-phase lifecycle:
//
//  Idle ──► Commission ──► Runtime
//    ▲____________│____________│
//
//  Idle
//    No threads running.  Devices live on the main (GUI) thread.
//
//  Commission
//    Each registered device gets its own HighPriority QThread.
//    The Widget obtains the typed runner via runnerFor(deviceId) and calls
//    requestConnect() / requestSingleShot() etc.  Results arrive back on
//    the GUI thread via QueuedConnection.
//
//  Runtime  (mergeToTaskThread = false, recommended)
//    Per-device threads are kept.  m_runtimeThread starts as coordinator.
//    The task's processing logic runs on m_runtimeThread; it coordinates
//    camera grabs and PLC I/O across the device threads via signals.
//
//  Runtime  (mergeToTaskThread = true)
//    All devices are detached from their per-device threads and moved into
//    m_runtimeThread.  Simpler but sequential — suitable only when devices
//    do not overlap IO.
//
//  Widget NEVER creates or destroys QThread objects.
//  Widget ONLY calls runner->requestXxx() and listens to runner signals.
// ─────────────────────────────────────────────────────────────────────────────

class TaskRunner : public QObject {
    Q_OBJECT

public:
    enum class Phase { Idle, Commission, Runtime };
    Q_ENUM(Phase)

    explicit TaskRunner(QObject *parent = nullptr);
    ~TaskRunner() override;

    // ── Device registration ───────────────────────────────────────────────────
    // Called by ITask::syncRunnersWithDevices() whenever devicesChanged fires.
    void registerDevice(const QString &deviceId,
                        std::shared_ptr<vc::device::IDevice> device);
    void unregisterDevice(const QString &deviceId);

    IDeviceRunner *runnerFor(const QString &deviceId) const;
    bool           hasRunner(const QString &deviceId) const;
    QStringList    registeredDeviceIds() const { return m_runners.keys(); }

    // ── Phase transitions ─────────────────────────────────────────────────────
    void enterCommission();
    void enterRuntime(bool mergeToTaskThread = false);
    void enterIdle();

    Phase   currentPhase()   const { return m_phase;          }
    QThread *runtimeThread() const { return m_runtimeThread;  }

signals:
    void requestDetach(QThread *dest);
    void phaseChanged(TaskRunner::Phase newPhase);

private:
    IDeviceRunner *createRunner(std::shared_ptr<vc::device::IDevice> device);

    Phase   m_phase{Phase::Idle};
    QThread *m_runtimeThread{nullptr};   // task coordinator thread


    QMap<QString, IDeviceRunner *> m_runners; // deviceId → runner
};

} // namespace vc::runtime

#endif // TASK_RUNNER_H
