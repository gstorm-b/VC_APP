# Device Thread Framework — Refactor Summary

> **Mục tiêu**: Mỗi device chạy trên một QThread riêng trong quá trình
> commission/config; có thể (tuỳ chọn) gộp về một thread chung của task khi
> chạy runtime. Widget **không** quản lý thread; **Task (qua TaskRunner)**
> quản lý toàn bộ vòng đời thread.
>
> **Trạng thái**: Refactor hoàn tất. Workers cũ trong `src/model/` đã được
> xoá. Widgets nhận Runner từ Task thay vì tự tạo Worker.

---

## 1. Cấu trúc dự án (sau refactor)

```
ncr_picking/
├── ncr_picking.pro                  # qmake project file
├── main.cpp / mainwindow.{h,cpp,ui}
├── windows_helper.h
├── 3rdparty/                        # Pylon, OpenCV, etc.
├── resrc/  resrc.qrc                # icons & resources
├── uml_diagram/
│   └── ncrn/
│       └── thread/                  # ◄ thread-management UML (NEW)
│           ├── class_diagram.puml
│           ├── sequence_commission.puml
│           ├── sequence_runtime.puml
│           ├── state_phase.puml
│           └── REFACTOR_SUMMARY.md  # ◄ tài liệu này
└── src/
    ├── device/                      # Device layer — pure I/O drivers
    │   ├── idevice.h                #   Abstract base (QObject)
    │   ├── idevice_config.h
    │   ├── irequest.h
    │   ├── device_manager.{h,cpp}   #   Global registry (no thread mgmt)
    │   ├── device_factory.{h,cpp}
    │   ├── communication_device.h
    │   ├── camera/
    │   │   ├── camera_device.h         # abstract CameraDevice
    │   │   ├── camera_basler_gige.{h,cpp}
    │   │   └── basler_define.h
    │   └── plc/
    │       ├── mc_protocol_device.{h,cpp}    # Mitsubishi MC protocol
    │       ├── mc_protocol_config.h
    │       ├── mc_request.h, mc_define.h, ...
    │       └── mitsubishi_ethernet_tcp_plc.{h,cpp}
    │
    ├── runtime/                     # ◄ NEW — thread / lifecycle layer
    │   ├── idevice_runner.h         #   Abstract runner interface
    │   ├── device_runner.h          #   Template base DeviceRunner<T>
    │   ├── camera_runner.h          #   CameraRunner (replaces CameraWorker)
    │   ├── mc_device_runner.h       #   McDeviceRunner (replaces McDeviceWorker)
    │   └── task_runner.{h,cpp}      #   Central thread manager owned by ITask
    │
    ├── model/                       # Task/Project model
    │   ├── itask.{h,cpp}            #   ◄ phase API + owns TaskRunner
    │   ├── itask_config.h
    │   ├── task_define.h
    │   ├── task_factory.{h,cpp}
    │   ├── task_localization.{h,cpp}    # ◄ uses runners (not workers)
    │   ├── task_localization_config.h
    │   ├── pick_and_place_task.h
    │   ├── project.{h,cpp}
    │   ├── project_repository.{h,cpp}
    │   ├── camera_map_entry.h
    │   └── isignal_group.h
    │   #  ✗ DELETED: camera_worker.h, mc_device_worker.h
    │
    ├── form/                        # UI / widgets layer
    │   ├── device_widget.h          #   IDeviceWidget interface
    │   ├── task_widget.h            #   ITaskWidget base
    │   ├── add_device_wizard.{h,cpp,ui}
    │   ├── new_project_dialog.{h,cpp,ui}
    │   ├── new_task_dialog.{h,cpp,ui}
    │   ├── navigate_form.{h,cpp,ui}
    │   ├── project_infor_setting.{h,cpp,ui}
    │   ├── task_form.{h,cpp,ui}
    │   ├── camera/
    │   │   ├── basler_camera_widget.{h,cpp,ui}      # ◄ injected runner
    │   │   └── basler_cam_select_dialog.{h,cpp,ui}
    │   ├── plc/
    │   │   ├── mc_protocol_device_widget.{h,cpp,ui} # ◄ injected runner
    │   │   ├── mc_ethernet_tcpip_widget.ui
    │   │   ├── mc_serial_port_widget.ui
    │   │   └── plc_mitsu_device_wizard.{h,cpp,ui}
    │   ├── pattern/                  # pattern editor widgets
    │   └── task/
    │       ├── localization_task_widget.{h,cpp,ui}  # ◄ beginCommission()
    │       ├── localization_dashboard_widget.{h,cpp,ui}
    │       ├── localization_calibration_widget.{h,cpp,ui}
    │       └── localization_patterns_widget.{h,cpp,ui}
    │
    ├── matching/                    # Image-matching algorithms
    │   ├── image_matcher.{h,cpp}
    │   ├── pattern_group_manager.{h,cpp}
    │   └── ...
    │
    ├── widgets/                     # Reusable Qt widgets (property browser, …)
    ├── libwg/                       # 3rd-party widgets (ADS dock, …)
    ├── logger/                      # AppLogger (LOG_DEV_DEBUG, LOG_DEV_ERR, …)
    ├── utils/
    └── app_settings/
```

---

## 2. Kiến trúc thread (Big Picture)

### 2.1 Ba layer chính

| Layer       | Vai trò                                                                |
|-------------|------------------------------------------------------------------------|
| **device**  | Driver I/O thuần (mở socket, gọi Pylon, polling timer). Không biết về thread. |
| **runtime** | Layer mới — quản lý vòng đời QThread cho mỗi device. Bao bọc device trong runner. |
| **model**   | `ITask` sở hữu một `TaskRunner`. `TaskRunner` sở hữu các `IDeviceRunner`. |
| **form**    | Widget chỉ gọi `runner->requestXxx()` và lắng signal từ runner. Không tạo thread. |

### 2.2 Quan hệ ownership

```
Project ─┬──► DeviceManager ──► IDevice (sống trên thread của runner)
         └──► ITask ──► TaskRunner ──► IDeviceRunner ──► QThread
                                                     └─► IDevice* (raw, không own)
```

- `DeviceManager` giữ `shared_ptr<IDevice>` (nguồn sống thật của device).
- `TaskRunner` giữ `IDeviceRunner*` (sở hữu QThread cho từng device).
- `IDeviceRunner` giữ raw pointer tới device — thread an toàn vì
  `attach()`/`detach()` luôn dùng `moveToThread()`.

### 2.3 State machine 3 phase

```
[*] ──► Idle ──► Commission ──► Runtime ──► Idle ──► [*]
         ▲          │              │           ▲
         └──────────┴──────────────┴───────────┘
              enterIdle() (bất cứ lúc nào)
```

| Phase           | Mô tả                                                            |
|-----------------|------------------------------------------------------------------|
| **Idle**        | Tất cả thread dừng. Device sống trên main thread.                |
| **Commission**  | Mỗi device → 1 QThread (HighPriority). Widget config, kết nối, single-shot. |
| **Runtime (per-device)** | Giữ thread mỗi device + 1 RT thread coordinator chạy task logic. |
| **Runtime (merged)**     | Tất cả device dồn vào RT thread. Tuần tự, đơn giản hơn nhưng chậm. |

---

## 3. Class Reference (chi tiết)

### 3.1 `runtime::IDeviceRunner` (`src/runtime/idevice_runner.h`)

Abstract interface cho mọi runner.

```cpp
class IDeviceRunner : public QObject {
    Q_OBJECT
public:
    virtual void start() = 0;
    virtual void stop()  = 0;
    virtual void attach() = 0;                  // moveToThread + wireSignals
    virtual void detach(QThread *dest = nullptr) = 0;
    virtual QThread             *workerThread() const = 0;
    virtual vc::device::IDevice *device()       const = 0;
    bool isAttached() const;
    bool isRunning()  const;
signals:
    void connectStatusChanged(vc::device::ConnectStatus status);
    void errorOccurred(const QString &msg);
protected:
    bool m_attached{false};
};
```

### 3.2 `runtime::DeviceRunner<T>` (`src/runtime/device_runner.h`)

Template base — xoá copy-paste giữa các runner cụ thể.

```cpp
template<typename TDevice>
class DeviceRunner : public IDeviceRunner {
public:
    explicit DeviceRunner(TDevice *device, QObject *parent = nullptr);
    void start()   override;     // m_thread->start(HighPriority)
    void stop()    override;     // quit + wait(3000)
    void attach()  override;     // m_device->moveToThread(m_thread) + wireSignals()
    void detach(QThread *dest)   // unwireSignals + moveToThread(dest|currentThread)
                  override;
    TDevice *typedDevice() const { return m_device; }
protected:
    virtual void wireSignals()   = 0;   // subclass connect QueuedConnection
    virtual void unwireSignals() = 0;
    TDevice *m_device{nullptr};
    QThread *m_thread{nullptr};         // owned (parented to runner)
};
```

### 3.3 `runtime::CameraRunner` (`src/runtime/camera_runner.h`)

```cpp
class CameraRunner : public DeviceRunner<vc::device::CameraDevice> {
public:
    void requestConnect();      // m_busy guard + emit sig_connect
    void requestDisconnect();
    void requestSingleShot();
    void requestApplyParams();
signals:
    void grabFinished(vc::device::GrabResult result);
    void parametersApplied(bool ok);
    // Internal queued triggers → camera thread
    void sig_connect(); void sig_disconnect();
    void sig_singleShot(); void sig_applyParams();
protected:
    void wireSignals()   override;   // QueuedConnection to/from device
    void unwireSignals() override;
private slots:
    void onConnectStatusChanged(...);  // m_busy=false + forward
    void onGrabFinished(...);
    // ...
private:
    bool m_busy{false};                // chống request chồng
};
```

### 3.4 `runtime::McDeviceRunner` (`src/runtime/mc_device_runner.h`)

```cpp
class McDeviceRunner : public DeviceRunner<vc::device::McProtocolDevice> {
public:
    void requestConnect();
    void requestDisconnect();
signals:
    void pollingUpdate(vc::device::McDeviceMap map);
    void requestFinished(vc::device::McResult result);
    void deviceMChanged(int, quint8, quint8);
    void deviceDChanged(int, qint16, qint16);
    void sig_connect(); void sig_disconnect();
protected:
    void wireSignals()   override;
    void unwireSignals() override;
private:
    bool m_busy{false};
};
```

### 3.5 `runtime::TaskRunner` (`src/runtime/task_runner.{h,cpp}`)

Class trung tâm — sở hữu mọi QThread của task.

```cpp
class TaskRunner : public QObject {
    Q_OBJECT
public:
    enum class Phase { Idle, Commission, Runtime };
    Q_ENUM(Phase)

    // Device registry
    void registerDevice(const QString &id, std::shared_ptr<IDevice> dev);
    void unregisterDevice(const QString &id);
    IDeviceRunner *runnerFor(const QString &id) const;
    bool           hasRunner(const QString &id) const;
    QStringList    registeredDeviceIds() const;

    // Phase transitions
    void enterCommission();
    void enterRuntime(bool mergeToTaskThread = false);
    void enterIdle();

    Phase   currentPhase()   const;
    QThread *runtimeThread() const;       // RT coordinator thread

signals:
    void phaseChanged(Phase newPhase);

private:
    IDeviceRunner *createRunner(std::shared_ptr<IDevice> dev);  // factory
    Phase   m_phase{Phase::Idle};
    QThread *m_runtimeThread{nullptr};
    QMap<QString, IDeviceRunner *> m_runners;
};
```

**Hành vi quan trọng**:
- `registerDevice()` — nếu `m_phase` đã là Commission/Runtime, runner mới
  được tự động `start() + attach()` ngay. Không phải gọi enterCommission lại.
- `enterRuntime(false)` — giữ thread mỗi device, chỉ thêm RT thread
  (NormalPriority).
- `enterRuntime(true)` — `detach + stop` mỗi runner, `moveToThread(m_runtimeThread)`,
  start RT thread (HighPriority).
- `enterIdle()` — `detach(nullptr)` đưa device về current thread + `stop()`
  toàn bộ runner + `quit()` RT thread.

### 3.6 `model::ITask` (`src/model/itask.{h,cpp}`)

```cpp
class ITask : public QObject {
    Q_OBJECT
public:
    // Devices
    void assignDevice(const QString &id);
    void unassignDevice(const QString &id);
    QStringList assignedDeviceIds() const;

    // ── Thread / phase API ─────────────────────────────────
    vc::runtime::TaskRunner *taskRunner() const;
    virtual void beginCommission();
    virtual void endCommission();
    virtual void beginRuntime(bool merge = false);
    virtual void endRuntime();
    virtual void stopAll();
    void resyncDevices();                     // sync runners ↔ assigned ids

signals:
    void devicesChanged();
    void commissionStarted();
    void commissionStopped();
    void runtimeStarted();
    void runtimeStopped();

protected:
    void syncRunnersWithDevices();            // register new + unregister orphaned

private:
    QStringList m_assignedDeviceIds;
    Project *m_proj{nullptr};
    vc::runtime::TaskRunner *m_taskRunner{
        new vc::runtime::TaskRunner(this) };  // owned, parent=this
};
```

### 3.7 `model::TaskLocalization` (`src/model/task_localization.{h,cpp}`)

Truy cập typed runner qua helper.

```cpp
class TaskLocalization : public ITask {
public:
    void setCameraDeviceId(const QString &id);
    void setMcDeviceId(const QString &id);

    vc::device::CameraDevice     *camera()   const;   // qua cameraRunner()
    vc::device::McProtocolDevice *mcDevice() const;

public slots:
    void setupTask();
    void executeLocalization();   // chạy trên RT thread

private:
    vc::runtime::CameraRunner   *cameraRunner(const QString &id) const;
    vc::runtime::McDeviceRunner *mcRunner(const QString &id)     const;

    QString m_cameraDeviceId;
    QString m_mcDeviceId;
    mtc::ImageMatcher m_matcher;
    mtc::PatternGroupManager *m_patternManager;
};
```

---

## 4. Flow theo phase

### 4.1 Commission flow

```
User mở task widget
  └─► LocalizationTaskWidget ctor → initWidget()
        └─► m_localizeTask->beginCommission()
              ├─► syncRunnersWithDevices()
              │     └─► for each assignedDeviceId:
              │           TaskRunner::registerDevice(id, device)
              │             └─► new CameraRunner / McDeviceRunner (parent=TaskRunner)
              └─► TaskRunner::enterCommission()
                    └─► for each runner:
                          runner.start()      // QThread.start(HighPriority)
                          runner.attach()     // device.moveToThread + wireSignals

User click device trong nav
  └─► LocalizationTaskWidget::getOrCreateDeviceConfigPage(id)
        ├─► runner = taskRunner()->runnerFor(id)
        └─► new BaslerCameraWidget(device, camRunner, ...)   // INJECT
              └─► connect(camRunner.connectStatusChanged → onCameraConnectStatusChanged)
              └─► connect(camRunner.grabFinished → onCameraGrabFinished)

User click [Connect Camera]
  └─► Widget::btn_connect_clicked()
        └─► m_runner->requestConnect()              // GUI thread
              └─► emit sig_connect()
                    └─► [QueuedConnection]
                          └─► CameraDevice::deviceConnect()  // Thread A
                                └─► emit connectStatusChanged
                                      └─► [QueuedConnection]
                                            └─► CameraRunner::onConnectStatusChanged
                                                  └─► emit connectStatusChanged
                                                        └─► Widget slot   (GUI)
```

### 4.2 Runtime flow (per-device mode, recommended)

```
User click [Start Runtime]
  └─► Task::beginRuntime(false)
        ├─► syncRunnersWithDevices()                // catch up new devices
        └─► TaskRunner::enterRuntime(false)
              ├─► (giữ nguyên thread mỗi device)
              └─► m_runtimeThread->start(NormalPriority)

executeLocalization() runs on RT thread:
  └─► camRunner->requestSingleShot()         [QueuedConnection → Thread A]
        └─► CameraDevice::grabSingleShot()
              └─► emit grabFinished           [QueuedConnection → RT thread]
                    └─► m_matcher.match(frame)
                          └─► mcRunner->pushRequest(...)  [QueuedConnection → Thread B]

PLC polling (autonomous):
  Thread B (PLC) timer → emit pollingUpdate → [QueuedConnection → RT] → react
```

### 4.3 Idle (cleanup)

```
LocalizationTaskWidget dtor
  └─► m_localizeTask->endCommission()
        └─► TaskRunner::enterIdle()
              ├─► for each runner:
              │     runner.detach(nullptr)   // moveToThread back to current
              │     runner.stop()            // QThread.quit + wait(3000)
              └─► m_runtimeThread.quit() + wait
```

---

## 5. Những thay đổi đã thực hiện

### 5.1 Files **mới** trong `src/runtime/`

| File                          | Vai trò                                              |
|-------------------------------|------------------------------------------------------|
| `idevice_runner.h`            | Abstract `IDeviceRunner` interface                   |
| `device_runner.h`             | Template `DeviceRunner<T>` — xoá copy-paste          |
| `camera_runner.h`             | `CameraRunner` — thay thế `CameraWorker`             |
| `mc_device_runner.h`          | `McDeviceRunner` — thay thế `McDeviceWorker`         |
| `task_runner.h` / `.cpp`      | `TaskRunner` — central thread manager                |

### 5.2 Files **sửa** trong `src/model/`

| File                    | Thay đổi                                                          |
|-------------------------|--------------------------------------------------------------------|
| `itask.h`               | Thêm phase API (`beginCommission/Runtime/...`), `m_taskRunner`, `resyncDevices()`, signals `commissionStarted/Stopped`, `runtimeStarted/Stopped`. |
| `itask.cpp`             | Implement phase methods + `syncRunnersWithDevices()` (register new + unregister orphan). |
| `task_localization.h`   | Bỏ `CameraWorker *m_worker`; thêm `cameraRunner(id)`, `mcRunner(id)`, `camera()`, `mcDevice()`, `setCameraDeviceId/setMcDeviceId`. |
| `task_localization.cpp` | Rewrite — dùng runner thay worker. `executeLocalization()` chạy trên RT thread. |

### 5.3 Files **sửa** trong `src/form/`

| File                                  | Thay đổi                                                  |
|---------------------------------------|-----------------------------------------------------------|
| `form/camera/basler_camera_widget.h`  | Constructor nhận `CameraRunner *runner`. Bỏ `m_worker`, thêm `m_runner` (không own). |
| `form/camera/basler_camera_widget.cpp`| Bỏ `new CameraWorker` + `moveToWorker`. Connect signals từ runner. Thay `m_worker->connectCamera()` → `m_runner->requestConnect()` v.v. |
| `form/plc/mc_protocol_device_widget.h`  | Constructor nhận `McDeviceRunner *runner`. Bỏ `m_worker`. |
| `form/plc/mc_protocol_device_widget.cpp`| Bỏ tạo worker. Connect `pollingUpdate` & `connectStatusChanged` từ runner. Click handler dùng `m_runner->requestConnect/Disconnect`. |
| `form/task/localization_task_widget.cpp`| `initWidget()` cuối hàm gọi `m_localizeTask->beginCommission()`. Destructor gọi `endCommission()`. `getOrCreateDeviceConfigPage()` lấy runner từ `taskRunner()->runnerFor(id)`, `qobject_cast` thành runner cụ thể, inject vào widget. `onTaskDevicesChanged()` gọi `resyncDevices()`. |

### 5.4 Files **xoá**

| File (đã xoá)                | Lý do                                                   |
|------------------------------|---------------------------------------------------------|
| `src/model/camera_worker.h`  | Thay bằng `runtime/camera_runner.h`                     |
| `src/model/mc_device_worker.h` | Thay bằng `runtime/mc_device_runner.h`                |

### 5.5 `ncr_picking.pro`

```diff
HEADERS += \
-   src/model/camera_worker.h \
-   src/model/mc_device_worker.h \
+   src/runtime/idevice_runner.h \
+   src/runtime/device_runner.h \
+   src/runtime/camera_runner.h \
+   src/runtime/mc_device_runner.h \
+   src/runtime/task_runner.h \
    ...

SOURCES += \
+   src/runtime/task_runner.cpp \
    ...
```

### 5.6 UML diagrams mới (`uml_diagram/ncrn/thread/`)

| File                       | Mô tả                                                        |
|----------------------------|--------------------------------------------------------------|
| `class_diagram.puml`       | 4 package (device, runtime, model, form). Quan hệ kế thừa & owns. |
| `sequence_commission.puml` | User assign device → beginCommission → connect → single-shot. |
| `sequence_runtime.puml`    | beginRuntime(false) → executeLocalization → grab + PLC polling parallel. |
| `state_phase.puml`         | State machine Idle ↔ Commission ↔ Runtime với entry actions. |
| `REFACTOR_SUMMARY.md`      | Tài liệu này.                                                |

---

## 6. Nguyên tắc thiết kế (rationale)

### 6.1 Tại sao Task quản lý thread, không phải Widget?

| Lý do                       | Hệ quả                                                       |
|-----------------------------|--------------------------------------------------------------|
| **Lifetime**                | Task sống xuyên suốt project; widget có thể bị huỷ/tạo lại. Nếu widget own thread, đóng widget = mất kết nối. |
| **Test/headless**           | Task có thể chạy không có widget (CLI/test). Logic thread không phụ thuộc UI. |
| **Đơn giản hoá Widget**     | Widget chỉ làm UI: `requestXxx()` + listen signal. Không có `QThread`, `moveToThread`, `wait`. |
| **Đa widget cùng task**     | Nhiều panel có thể nhìn cùng 1 device qua cùng 1 runner — không trùng thread. |

### 6.2 Tại sao có Runner (không gọi thẳng device)?

- Runner là **GUI-thread proxy**: signals được forward qua QueuedConnection
  → guarantee chạy trên thread của runner (GUI), an toàn cho UI slots.
- Runner giữ `m_busy` → idempotent với click double.
- Runner tách `attach/detach` ↔ `start/stop` → có thể giữ thread chạy mà
  tạm thời tách device (chuyển từ Commission sang Runtime merged).

### 6.3 Tại sao template `DeviceRunner<T>`?

Cũ: `CameraWorker` & `McDeviceWorker` ~80% giống nhau (start/stop/move/wire).
Mới: `DeviceRunner<T>` xử lý generic; subclass chỉ implement `wireSignals()`/
`unwireSignals()` đặc thù device.

### 6.4 Per-device vs merged runtime

| Mode             | Khi dùng                                                         |
|------------------|------------------------------------------------------------------|
| **per-device** (default) | Camera grab + PLC poll **song song** trên 2 thread. Throughput cao nhất. RT thread chỉ coordinate. |
| **merged**       | I/O không trùng nhau (vd. chỉ camera, hoặc tuần tự cam→plc). Đơn giản hơn, ít thread switch. |

---

## 7. Quy ước & khuyến nghị

1. **Widget**: KHÔNG bao giờ gọi `moveToThread`, `new QThread`, `start/quit`.
   Chỉ gọi `runner->requestXxx()` và `connect(runner, …)`.
2. **Task**: KHÔNG gọi thẳng `device.deviceConnect()`. Luôn qua
   `taskRunner()->runnerFor(id)`.
3. **Phase transition**: Luôn gọi qua `ITask::beginCommission/endCommission/
   beginRuntime/endRuntime` — không gọi `TaskRunner` trực tiếp từ Widget.
4. **Thread affinity**: Sau `attach()`, device sống trên QThread của runner.
   Không gọi getter/setter từ thread khác (trừ pure const getter).
5. **Signal connection**: Mọi connection trong `wireSignals()` PHẢI dùng
   `Qt::QueuedConnection` (cross-thread).
6. **Cleanup**: TaskRunner destructor tự gọi `enterIdle()` → an toàn nếu
   user quên.

---

## 8. Tham khảo nhanh code

### Widget khởi tạo có runner

```cpp
// Trong LocalizationTaskWidget::getOrCreateDeviceConfigPage(id):
auto *taskRunner = m_localizeTask->taskRunner();
auto *runner     = taskRunner ? taskRunner->runnerFor(deviceId) : nullptr;

switch (device->deviceType()) {
case DeviceType::Camera: {
    auto *camRunner = qobject_cast<CameraRunner *>(runner);
    page = new BaslerCameraWidget(device, camRunner, nullptr, this);
    break;
}
case DeviceType::McDevice: {
    auto *mcRunner = qobject_cast<McDeviceRunner *>(runner);
    page = new McProtocolDeviceWidget(device, mcRunner, nullptr, this);
    break;
}
}
```

### Widget gọi runner

```cpp
void BaslerCameraWidget::btn_connect_clicked() {
    if (!m_runner) return;
    if (!m_camera->isDeviceConnected())
        m_runner->requestConnect();
    else
        m_runner->requestDisconnect();
}

void BaslerCameraWidget::btn_trigger_clicked() {
    if (m_camera->isDeviceConnected() && m_runner)
        m_runner->requestSingleShot();
}
```

### Widget connect signal từ runner

```cpp
connect(m_runner, &CameraRunner::connectStatusChanged,
        this,     &BaslerCameraWidget::onCameraConnectStatusChanged);
connect(m_runner, &CameraRunner::grabFinished,
        this,     &BaslerCameraWidget::onCameraGrabFinished);
```

### Task chạy executeLocalization (RT thread)

```cpp
void TaskLocalization::executeLocalization() {
    if (!m_isValid) return;

    auto *camRunner = cameraRunner(m_cameraDeviceId);
    if (camRunner) camRunner->requestSingleShot();
    // grabFinished signal sẽ về trên RT thread (vì runner ở GUI? – tuỳ
    // QueuedConnection target). Trong merged-runtime, runner cũng được
    // move tới RT thread → signal về trực tiếp).
}
```

---

## 9. Checklist khi thêm device mới

1. **Device class** trong `src/device/<category>/` kế thừa `IDevice`
   (hoặc abstract con như `CameraDevice`).
2. **Runner class** trong `src/runtime/` kế thừa `DeviceRunner<TDevice>`,
   implement `wireSignals()` / `unwireSignals()` và `requestXxx()` API.
3. **Factory**: thêm case trong `TaskRunner::createRunner()` switch theo
   `DeviceType`.
4. **DeviceType enum**: cập nhật `src/device/idevice.h`.
5. **Widget** trong `src/form/<category>/`: nhận runner qua constructor,
   không tạo worker.
6. **TaskWidget**: thêm case trong `getOrCreateDeviceConfigPage()` switch.
7. **`ncr_picking.pro`**: thêm header/source mới.
8. **UML**: cập nhật `class_diagram.puml`.

---

_Tài liệu này tổng hợp toàn bộ refactor thread-management framework hoàn tất tháng 5/2026._
