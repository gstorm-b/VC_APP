#include <QtTest/QtTest>

#include <QJsonArray>
#include <QJsonObject>

#include <memory>

#include "device/camera/camera_basler_gige.h"
#include "device/device_capabilities.h"
#include "device/device_factory.h"
#include "device/device_registry.h"
#include "device/output_device/vision_tcpip_device.h"
#include "device/plc/mc_context_3e.h"
#include "device/plc/mc_protocol_device.h"
#include "device/robot/kawasaki_robot_device.h"
#include "device/robot/nachi_robot_device.h"
#include "model/localization_signal_mapper.h"
#include "model/localization_recovery_policy.h"
#include "model/task_factory.h"
#include "model/task_localization.h"
#include "model/task_state_machine.h"
#include "runtime/device_command.h"
#include "runtime/camera_runner.h"
#include "runtime/plc_runner.h"
#include "runtime/task_runner.h"
#include "runtime/vision_output_runner.h"

using namespace vc::device;
using namespace vc::model;
using namespace vc::runtime;

namespace {

QJsonObject baseDeviceJson(const QString &id,
                           const QString &name,
                           DeviceType type,
                           const QJsonObject &config)
{
    return {
        { DEVICE_JSK_ID, id },
        { DEVICE_JSK_NAME, name },
        { DEVICE_JSK_TYPE, DeviceTypeToString(type) },
        { DEVICE_JSK_CONFIG, config }
    };
}

QJsonObject baslerDeviceJson(const QString &id = QStringLiteral("cam1"))
{
    BaslerGigeCfg cfg;
    cfg.m_modelName = QStringLiteral("ContractTestModel");
    cfg.m_userDefinedName = QStringLiteral("ContractTestCamera");
    cfg.m_serialNumber = QStringLiteral("SN-ARCH-001");
    cfg.m_ipAddress = QStringLiteral("192.168.10.10");
    cfg.m_autoExposureMode = vc::device::basler::BaslerExposureMode::Exposure_Off;

    QJsonObject obj = baseDeviceJson(id,
                                     QStringLiteral("Basler Camera"),
                                     DeviceType::Camera,
                                     cfg.toJson());
    obj[DEVICE_JSK_CAM_TYPE] = CameraTypeToString(CameraType::BaslerGigE);
    return obj;
}

QJsonObject plcDeviceJson(const QString &id = QStringLiteral("plc1"))
{
    McProtocolConfig cfg;
    cfg.configMcProtocol(vc::device::mc::McFrameType::Frame_3E,
                         vc::device::mc::McDataCode::Binary);
    return baseDeviceJson(id,
                          QStringLiteral("Mitsubishi MC PLC"),
                          DeviceType::PLC,
                          cfg.toJson());
}

QJsonObject visionOutputDeviceJson(const QString &id = QStringLiteral("vout1"))
{
    VisionTcpipDeviceCfg cfg;
    cfg.m_listenAddress = QStringLiteral("127.0.0.1");
    cfg.m_mainPort = 25001;
    cfg.m_heartbeatPort = 25002;
    cfg.m_heartbeatIntervalMs = 200;
    cfg.m_heartbeatTimeoutMs = 1000;
    return baseDeviceJson(id,
                          QStringLiteral("Vision TCP/IP Output"),
                          DeviceType::VisionOutput,
                          cfg.toJson());
}

QJsonObject kawasakiRobotDeviceJson(const QString &id = QStringLiteral("robot1"))
{
    KawasakiRobotCfg cfg;
    return baseDeviceJson(id,
                          QStringLiteral("Kawasaki Robot"),
                          DeviceType::Robot,
                          cfg.toJson());
}

QJsonObject nachiRobotDeviceJson(const QString &id = QStringLiteral("robot2"))
{
    NachiRobotCfg cfg;
    return baseDeviceJson(id,
                          QStringLiteral("Nachi Robot"),
                          DeviceType::Robot,
                          cfg.toJson());
}

QJsonObject unsupportedCameraJson(CameraType type)
{
    QJsonObject cfg;
    cfg[DEVICE_JSK_CAM_TYPE] = CameraTypeToString(type);
    QJsonObject obj = baseDeviceJson(QStringLiteral("unsupported_cam"),
                                     QStringLiteral("Unsupported Camera"),
                                     DeviceType::Camera,
                                     cfg);
    obj[DEVICE_JSK_CAM_TYPE] = CameraTypeToString(type);
    return obj;
}

QJsonObject unsupportedVisionSerialJson()
{
    QJsonObject cfg;
    cfg[DEVICE_JSK_VOUT_TYPE] = VisionOutputTypeToString(VisionOutputType::VisionSerial);
    return baseDeviceJson(QStringLiteral("unsupported_vout"),
                          QStringLiteral("Unsupported Vision Serial"),
                          DeviceType::VisionOutput,
                          cfg);
}

QJsonObject unsupportedHuayanRobotJson()
{
    QJsonObject cfg;
    cfg[DEVICE_JSK_ROBOT_TYPE] = RobotTypeToString(RobotType::Huayan);
    return baseDeviceJson(QStringLiteral("unsupported_robot"),
                          QStringLiteral("Unsupported Huayan Robot"),
                          DeviceType::Robot,
                          cfg);
}

QJsonObject localizationTaskJson()
{
    TaskLocalizeConfig cfg;
    QJsonObject taskObj;
    taskObj["id"] = QStringLiteral("task-localization-1");
    taskObj["name"] = QStringLiteral("Localization Task");
    taskObj["taskType"] = taskTypeToString(TaskType::LocalizationTask);
    taskObj["cameraSourceType"] = qenumToString(CameraSourceType::Source_Owned);
    taskObj["ownedCameraId"] = QString();
    taskObj["assignedDeviceIds"] = QJsonArray();
    taskObj["taskConfig"] = cfg.toJson();
    taskObj["patternManager"] = QJsonObject{{"groups", QJsonArray()}};
    return taskObj;
}

DeviceCommandResult findResultByCommandId(const QSignalSpy &spy, const QString &commandId)
{
    for (const QList<QVariant> &signalArgs : spy) {
        const DeviceCommandResult result =
            qvariant_cast<DeviceCommandResult>(signalArgs.at(0));
        if (result.commandId == commandId) {
            return result;
        }
    }
    return DeviceCommandResult();
}

} // namespace

class TaskLocalizationProbe : public TaskLocalization {
public:
    using TaskLocalization::TaskLocalization;
    using ITask::transitionTaskState;
};

class ArchitectureContractTest : public QObject {
    Q_OBJECT

private slots:
    void test_device_factory_supported_subtypes()
    {
        std::unique_ptr<IDevice> camera(DeviceFactory::fromJson(baslerDeviceJson()));
        QVERIFY(camera != nullptr);
        QCOMPARE(camera->deviceType(), DeviceType::Camera);
        QVERIFY(qobject_cast<BaslerGigECamera *>(camera.get()) != nullptr);

        std::unique_ptr<IDevice> plc(DeviceFactory::fromJson(plcDeviceJson()));
        QVERIFY(plc != nullptr);
        QCOMPARE(plc->deviceType(), DeviceType::PLC);
        QVERIFY(qobject_cast<McProtocolDevice *>(plc.get()) != nullptr);

        std::unique_ptr<IDevice> output(DeviceFactory::fromJson(visionOutputDeviceJson()));
        QVERIFY(output != nullptr);
        QCOMPARE(output->deviceType(), DeviceType::VisionOutput);
        QVERIFY(qobject_cast<VisionTcpipDevice *>(output.get()) != nullptr);

        std::unique_ptr<IDevice> kawasaki(DeviceFactory::fromJson(kawasakiRobotDeviceJson()));
        QVERIFY(kawasaki != nullptr);
        QCOMPARE(kawasaki->deviceType(), DeviceType::Robot);
        QVERIFY(qobject_cast<KawasakiRobotDevice *>(kawasaki.get()) != nullptr);

        std::unique_ptr<IDevice> nachi(DeviceFactory::fromJson(nachiRobotDeviceJson()));
        QVERIFY(nachi != nullptr);
        QCOMPARE(nachi->deviceType(), DeviceType::Robot);
        QVERIFY(qobject_cast<NachiRobotDevice *>(nachi.get()) != nullptr);
    }

    void test_device_factory_unsupported_declared_subtypes_return_null()
    {
        std::unique_ptr<IDevice> realsense(
            DeviceFactory::fromJson(unsupportedCameraJson(CameraType::Realsense)));
        QVERIFY(realsense == nullptr);

        std::unique_ptr<IDevice> baslerUsb(
            DeviceFactory::fromJson(unsupportedCameraJson(CameraType::BaslerUSB)));
        QVERIFY(baslerUsb == nullptr);

        std::unique_ptr<IDevice> serialOutput(
            DeviceFactory::fromJson(unsupportedVisionSerialJson()));
        QVERIFY(serialOutput == nullptr);

        std::unique_ptr<IDevice> huayan(
            DeviceFactory::fromJson(unsupportedHuayanRobotJson()));
        QVERIFY(huayan == nullptr);
    }

    void test_concrete_device_configs_round_trip_json()
    {
        BaslerGigeCfg baslerCfg;
        baslerCfg.m_modelName = QStringLiteral("Model");
        baslerCfg.m_serialNumber = QStringLiteral("Serial");
        baslerCfg.m_ipAddress = QStringLiteral("10.0.0.2");
        baslerCfg.m_autoExposureMode = vc::device::basler::BaslerExposureMode::Exposure_Off;
        BaslerGigeCfg baslerRestored;
        QVERIFY(baslerRestored.fromJson(baslerCfg.toJson()));
        QCOMPARE(baslerRestored.cameraType(), CameraType::BaslerGigE);
        QCOMPARE(baslerRestored.m_ipAddress, baslerCfg.m_ipAddress);

        McProtocolConfig plcCfg;
        QVERIFY(plcCfg.configMcProtocol(vc::device::mc::McFrameType::Frame_3E,
                                        vc::device::mc::McDataCode::Binary));
        McProtocolConfig plcRestored;
        QVERIFY(plcRestored.fromJson(plcCfg.toJson()));
        QCOMPARE(plcRestored.plcType(), PlcType::MitsubishiMc);
        QCOMPARE(plcRestored.currentFrameType(), vc::device::mc::McFrameType::Frame_3E);

        VisionTcpipDeviceCfg outputCfg;
        outputCfg.m_listenAddress = QStringLiteral("127.0.0.1");
        outputCfg.m_mainPort = 26001;
        outputCfg.m_heartbeatPort = 26002;
        VisionTcpipDeviceCfg outputRestored;
        QVERIFY(outputRestored.fromJson(outputCfg.toJson()));
        QCOMPARE(outputRestored.visionOutputType(), VisionOutputType::VisionTCPIP);
        QCOMPARE(outputRestored.m_mainPort, outputCfg.m_mainPort);

        KawasakiRobotCfg kawasakiCfg;
        KawasakiRobotCfg kawasakiRestored;
        QVERIFY(kawasakiRestored.fromJson(kawasakiCfg.toJson()));
        QCOMPARE(kawasakiRestored.robotType(), RobotType::Kawasaki);

        NachiRobotCfg nachiCfg;
        NachiRobotCfg nachiRestored;
        QVERIFY(nachiRestored.fromJson(nachiCfg.toJson()));
        QCOMPARE(nachiRestored.robotType(), RobotType::Nachi);
    }

    void test_task_localize_config_round_trips_device_bindings()
    {
        TaskLocalizeConfig cfg;
        cfg.d->m_deviceBindings.setPrimaryPlcDeviceId(QStringLiteral("plc1"));
        cfg.d->m_deviceBindings.setVisionOutputDeviceId(QStringLiteral("vout1"));
        cfg.d->m_deviceBindings.setCameraNumberMap({
            {1, QStringLiteral("cam1")},
            {2, QStringLiteral("cam2")}
        });

        TaskLocalizeConfig restored;
        QVERIFY(restored.fromJson(cfg.toJson()));
        QCOMPARE(restored.d->m_deviceBindings.primaryPlcDeviceId(),
                 QStringLiteral("plc1"));
        QCOMPARE(restored.d->m_deviceBindings.visionOutputDeviceId(),
                 QStringLiteral("vout1"));
        QCOMPARE(restored.d->m_deviceBindings.cameraDeviceId(1),
                 QStringLiteral("cam1"));
        QCOMPARE(restored.d->m_deviceBindings.cameraDeviceId(2),
                 QStringLiteral("cam2"));
    }

    void test_localization_signal_mapper_translates_configured_tags()
    {
        TaskLocalizeConfig cfg;
        cfg.setnActiveCamera(QStringLiteral("D100"));
        cfg.setbExecuteTrigger(QStringLiteral("M10"));

        LocalizationSignalMapper mapper;
        mapper.configure(cfg);

        const QList<LocalizationSignalEvent> events = mapper.mapValues({
            {QStringLiteral("D100"), 2},
            {QStringLiteral("M10"), true},
            {QStringLiteral("M999"), false}
        });

        QCOMPARE(events.size(), 2);
        QCOMPARE(events.at(0).name, QStringLiteral("nActiveCamera"));
        QCOMPARE(events.at(0).value.toInt(), 2);
        QCOMPARE(events.at(1).name, QStringLiteral("bExecuteTrigger"));
        QCOMPARE(events.at(1).value.toBool(), true);
    }

    void test_task_factory_restores_localization_task()
    {
        std::unique_ptr<ITask> task(TaskFactory::fromJson(localizationTaskJson()));
        QVERIFY(task != nullptr);
        QCOMPARE(task->taskType(), TaskType::LocalizationTask);
        QCOMPARE(task->id(), QStringLiteral("task-localization-1"));
        QCOMPARE(task->name(), QStringLiteral("Localization Task"));
        QVERIFY(qobject_cast<TaskLocalization *>(task.get()) != nullptr);

        task->stopAll();
    }

    void test_task_runner_creates_supported_family_runners()
    {
        TaskRunner runner;

        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cam_runner"), QStringLiteral("Camera Runner Device"));
        runner.registerDevice(camera->id(), camera);
        QVERIFY(runner.hasRunner(camera->id()));
        QVERIFY(qobject_cast<CameraRunner *>(runner.runnerFor(camera->id())) != nullptr);

        auto plc = std::make_shared<McProtocolDevice>(
            QStringLiteral("plc_runner"), QStringLiteral("PLC Runner Device"));
        runner.registerDevice(plc->id(), plc);
        QVERIFY(runner.hasRunner(plc->id()));
        QVERIFY(qobject_cast<PlcRunner *>(runner.runnerFor(plc->id())) != nullptr);

        auto output = std::make_shared<VisionTcpipDevice>(
            QStringLiteral("vout_runner"), QStringLiteral("Output Runner Device"));
        runner.registerDevice(output->id(), output);
        QVERIFY(runner.hasRunner(output->id()));
        QVERIFY(qobject_cast<VisionOutputRunner *>(runner.runnerFor(output->id())) != nullptr);

        auto robot = std::make_shared<KawasakiRobotDevice>(
            QStringLiteral("robot_runner"), QStringLiteral("Robot Runner Device"));
        runner.registerDevice(robot->id(), robot);
        QVERIFY(!runner.hasRunner(robot->id()));
        QVERIFY(runner.runnerFor(robot->id()) == nullptr);

        runner.enterIdle();
    }

    void test_device_capability_interfaces_match_supported_families()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cap_cam"), QStringLiteral("Capability Camera"));
        auto plc = std::make_shared<McProtocolDevice>(
            QStringLiteral("cap_plc"), QStringLiteral("Capability PLC"));
        auto output = std::make_shared<VisionTcpipDevice>(
            QStringLiteral("cap_vout"), QStringLiteral("Capability Vision Output"));
        auto robot = std::make_shared<KawasakiRobotDevice>(
            QStringLiteral("cap_robot"), QStringLiteral("Capability Robot"));

        QVERIFY(dynamic_cast<IImageSourceDevice *>(camera.get()) != nullptr);
        QVERIFY(dynamic_cast<IPlcTagProvider *>(plc.get()) != nullptr);
        QVERIFY(dynamic_cast<IDigitalIoProvider *>(plc.get()) != nullptr);
        QVERIFY(dynamic_cast<IWordIoProvider *>(plc.get()) != nullptr);
        QVERIFY(dynamic_cast<IResultOutputDevice *>(output.get()) != nullptr);

        QVERIFY(dynamic_cast<IPlcTagProvider *>(camera.get()) == nullptr);
        QVERIFY(dynamic_cast<IPlcTagProvider *>(output.get()) == nullptr);
        QVERIFY(dynamic_cast<IPlcTagProvider *>(robot.get()) == nullptr);
    }

    void test_device_registry_lists_supported_subtypes()
    {
        QCOMPARE(DeviceRegistry::displayNamesFor(DeviceType::Camera),
                 QStringList{CameraTypeToString(CameraType::BaslerGigE)});
        QCOMPARE(DeviceRegistry::displayNamesFor(DeviceType::PLC),
                 QStringList{PlcTypeToString(PlcType::MitsubishiMc)});
        QCOMPARE(DeviceRegistry::displayNamesFor(DeviceType::VisionOutput),
                 QStringList{VisionOutputTypeToString(VisionOutputType::VisionTCPIP)});

        const DeviceRegistryEntry *cameraEntry =
            DeviceRegistry::find(DeviceType::Camera,
                                 CameraTypeToString(CameraType::BaslerGigE));
        QVERIFY(cameraEntry != nullptr);
        QCOMPARE(cameraEntry->configJsonKey, QStringLiteral(DEVICE_JSK_CAM_TYPE));
    }

    void test_vision_tcpip_runtime_state_is_not_persisted()
    {
        VisionTcpipDevice device(QStringLiteral("vision_state"), QStringLiteral("Vision State"));

        const VisionTcpipRuntimeState runtimeState = device.runtimeState();
        QCOMPARE(runtimeState.mainClientConnected, false);
        QCOMPARE(runtimeState.heartbeatClientConnected, false);

        const VisionTcpipDiagnostics diagnostics = device.diagnostics();
        QCOMPARE(diagnostics.mainPayloadsReceived, quint64(0));
        QCOMPARE(diagnostics.resultPayloadsSent, quint64(0));

        const QJsonObject json = device.toJson();
        QVERIFY(!json.contains(QStringLiteral("mainClientConnected")));
        QVERIFY(!json.contains(QStringLiteral("heartbeatClientConnected")));
        QVERIFY(!json.contains(QStringLiteral("lastError")));
        QVERIFY(json.contains(QStringLiteral(DEVICE_JSK_CONFIG)));
    }

    void test_camera_runner_standard_command_rejects_overlap()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cmd_cam"), QStringLiteral("Command Camera"));
        CameraRunner runner(camera.get());

        const DeviceCommand first = DeviceCommand::create(
            DeviceCommandKind::CameraSingleShot,
            camera->id());
        const DeviceCommandResult firstResult = runner.submitCommand(first);
        QVERIFY(firstResult.status == DeviceCommandResultStatus::Accepted);
        QVERIFY(firstResult.code == DeviceCommandResultCode::None);
        QCOMPARE(firstResult.commandId, first.id);

        const DeviceCommand second = DeviceCommand::create(
            DeviceCommandKind::Connect,
            camera->id());
        const DeviceCommandResult busyResult = runner.submitCommand(second);
        QVERIFY(busyResult.status == DeviceCommandResultStatus::Rejected);
        QVERIFY(busyResult.code == DeviceCommandResultCode::Busy);
        QCOMPARE(busyResult.commandId, second.id);
    }

    void test_camera_runner_standard_command_rejects_invalid_target_and_kind()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cmd_cam_target"), QStringLiteral("Command Camera Target"));
        CameraRunner runner(camera.get());

        const DeviceCommand wrongTarget = DeviceCommand::create(
            DeviceCommandKind::Connect,
            QStringLiteral("another_device"));
        const DeviceCommandResult wrongTargetResult = runner.submitCommand(wrongTarget);
        QVERIFY(wrongTargetResult.status == DeviceCommandResultStatus::Rejected);
        QVERIFY(wrongTargetResult.code == DeviceCommandResultCode::InvalidTarget);

        const DeviceCommand unsupported = DeviceCommand::create(
            DeviceCommandKind::Unknown,
            camera->id());
        const DeviceCommandResult unsupportedResult = runner.submitCommand(unsupported);
        QVERIFY(unsupportedResult.status == DeviceCommandResultStatus::Rejected);
        QVERIFY(unsupportedResult.code == DeviceCommandResultCode::UnsupportedCommand);
    }

    void test_camera_runner_standard_command_emits_success_result()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cmd_cam_success"), QStringLiteral("Command Camera Success"));
        CameraRunner runner(camera.get());

        QSignalSpy finishedSpy(&runner, &CameraRunner::commandFinished);
        QVERIFY(finishedSpy.isValid());

        const DeviceCommand singleShotCmd = DeviceCommand::create(
            DeviceCommandKind::CameraSingleShot,
            camera->id(),
            800);
        const DeviceCommandResult accepted = runner.submitCommand(singleShotCmd);
        QCOMPARE(accepted.status, DeviceCommandResultStatus::Accepted);

        vc::device::GrabResult done;
        done.isGrabSuccess = true;
        done.msg = QStringLiteral("single shot success");
        QVERIFY(QMetaObject::invokeMethod(
            &runner,
            "onGrabFinished",
            Qt::DirectConnection,
            Q_ARG(vc::device::GrabResult, done)));

        QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() >= 1, 1000);
        const DeviceCommandResult success =
            findResultByCommandId(finishedSpy, singleShotCmd.id);
        QCOMPARE(success.commandId, singleShotCmd.id);
        QCOMPARE(success.status, DeviceCommandResultStatus::Succeeded);
        QCOMPARE(success.code, DeviceCommandResultCode::None);
    }

    void test_camera_runner_standard_command_timeout_result()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cmd_cam_timeout"), QStringLiteral("Command Camera Timeout"));
        CameraRunner runner(camera.get());

        QSignalSpy finishedSpy(&runner, &CameraRunner::commandFinished);
        QVERIFY(finishedSpy.isValid());

        const DeviceCommand singleShotCmd = DeviceCommand::create(
            DeviceCommandKind::CameraSingleShot,
            camera->id(),
            40);
        const DeviceCommandResult accepted = runner.submitCommand(singleShotCmd);
        QCOMPARE(accepted.status, DeviceCommandResultStatus::Accepted);

        QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() >= 1, 1200);
        const DeviceCommandResult timeoutResult =
            findResultByCommandId(finishedSpy, singleShotCmd.id);
        QCOMPARE(timeoutResult.commandId, singleShotCmd.id);
        QCOMPARE(timeoutResult.status, DeviceCommandResultStatus::Failed);
        QCOMPARE(timeoutResult.code, DeviceCommandResultCode::TimedOut);
    }

    void test_camera_runner_standard_command_queue_policy()
    {
        auto camera = std::make_shared<BaslerGigECamera>(
            QStringLiteral("cmd_cam_queue"), QStringLiteral("Command Camera Queue"));
        CameraRunner runner(camera.get());

        QSignalSpy finishedSpy(&runner, &CameraRunner::commandFinished);
        QVERIFY(finishedSpy.isValid());

        const DeviceCommand first = DeviceCommand::create(
            DeviceCommandKind::CameraSingleShot,
            camera->id(),
            600);
        const DeviceCommand second = DeviceCommand::create(
            DeviceCommandKind::CameraSingleShot,
            camera->id(),
            600);
        const DeviceCommand busyRejected = DeviceCommand::create(
            DeviceCommandKind::Connect,
            camera->id(),
            600);

        QCOMPARE(runner.submitCommand(first).status, DeviceCommandResultStatus::Accepted);
        QCOMPARE(runner.submitCommand(second).status, DeviceCommandResultStatus::Accepted);
        const DeviceCommandResult rejectedResult = runner.submitCommand(busyRejected);
        QCOMPARE(rejectedResult.status, DeviceCommandResultStatus::Rejected);
        QCOMPARE(rejectedResult.code, DeviceCommandResultCode::Busy);

        vc::device::GrabResult firstDone;
        firstDone.isGrabSuccess = true;
        firstDone.msg = QStringLiteral("first done");
        QVERIFY(QMetaObject::invokeMethod(
            &runner,
            "onGrabFinished",
            Qt::DirectConnection,
            Q_ARG(vc::device::GrabResult, firstDone)));

        vc::device::GrabResult secondDone;
        secondDone.isGrabSuccess = true;
        secondDone.msg = QStringLiteral("second done");
        QVERIFY(QMetaObject::invokeMethod(
            &runner,
            "onGrabFinished",
            Qt::DirectConnection,
            Q_ARG(vc::device::GrabResult, secondDone)));

        QTRY_VERIFY_WITH_TIMEOUT(finishedSpy.count() >= 3, 1500);

        const DeviceCommandResult firstResult = findResultByCommandId(finishedSpy, first.id);
        QCOMPARE(firstResult.commandId, first.id);
        QCOMPARE(firstResult.status, DeviceCommandResultStatus::Succeeded);

        const DeviceCommandResult secondResult = findResultByCommandId(finishedSpy, second.id);
        QCOMPARE(secondResult.commandId, second.id);
        QCOMPARE(secondResult.status, DeviceCommandResultStatus::Succeeded);

        const DeviceCommandResult busyResult =
            findResultByCommandId(finishedSpy, busyRejected.id);
        QCOMPARE(busyResult.commandId, busyRejected.id);
        QCOMPARE(busyResult.status, DeviceCommandResultStatus::Rejected);
        QCOMPARE(busyResult.code, DeviceCommandResultCode::Busy);
    }

    void test_localization_recovery_policy_defaults_match_runtime_spec()
    {
        const LocalizationRecoveryPolicy cameraPolicy = defaultCameraRecoveryPolicy();
        QCOMPARE(cameraPolicy.roleName, QStringLiteral("camera"));
        QCOMPARE(cameraPolicy.maxRetries, 10);
        QCOMPARE(cameraPolicy.retryIntervalMs, 5000);
        QCOMPARE(cameraPolicy.connectTimeoutMs, 3000);

        const LocalizationRecoveryPolicy plcPolicy = defaultPlcRecoveryPolicy();
        QCOMPARE(plcPolicy.roleName, QStringLiteral("primary_plc"));
        QCOMPARE(plcPolicy.maxRetries, 10);
        QCOMPARE(plcPolicy.retryIntervalMs, 5000);

        const LocalizationRecoveryPolicy visionPolicy = defaultVisionOutputRecoveryPolicy();
        QCOMPARE(visionPolicy.roleName, QStringLiteral("vision_output"));
        QCOMPARE(visionPolicy.maxRetries, 10);
        QCOMPARE(visionPolicy.retryIntervalMs, 5000);
    }

    void test_localization_recovery_policy_decision_rules()
    {
        LocalizationRecoveryPolicy policy = defaultCameraRecoveryPolicy();
        policy.maxRetries = 2;

        QCOMPARE(decideRecoveryAction(policy,
                                      vc::device::ConnectStatus::Connected,
                                      0,
                                      false),
                 LocalizationRecoveryAction::Ignore);

        QCOMPARE(decideRecoveryAction(policy,
                                      vc::device::ConnectStatus::LostConnected,
                                      0,
                                      false),
                 LocalizationRecoveryAction::RetryScheduled);

        QCOMPARE(decideRecoveryAction(policy,
                                      vc::device::ConnectStatus::LostConnected,
                                      1,
                                      true),
                 LocalizationRecoveryAction::Ignore);

        QCOMPARE(decideRecoveryAction(policy,
                                      vc::device::ConnectStatus::ConnectFailed,
                                      2,
                                      false),
                 LocalizationRecoveryAction::EscalateFault);
    }

    void test_localization_recovery_fault_message_includes_role_and_retry_history()
    {
        LocalizationRecoveryPolicy policy = defaultVisionOutputRecoveryPolicy();
        policy.maxRetries = 10;
        policy.retryIntervalMs = 5000;
        policy.connectTimeoutMs = 3000;

        const QString message = buildRecoveryFaultMessage(
            policy,
            QStringLiteral("vout-01"),
            vc::device::ConnectStatus::ConnectFailed,
            10);

        QVERIFY(message.contains(QStringLiteral("role=vision_output")));
        QVERIFY(message.contains(QStringLiteral("deviceId=vout-01")));
        QVERIFY(message.contains(QStringLiteral("status=ConnectFailed")));
        QVERIFY(message.contains(QStringLiteral("retries=10/10")));
        QVERIFY(message.contains(QStringLiteral("intervalMs=5000")));
        QVERIFY(message.contains(QStringLiteral("timeoutMs=3000")));
    }

    void test_task_state_machine_transition_rules()
    {
        QVERIFY(canTransitionTaskState(TaskState::Idle, TaskState::CommissionStarting));
        QVERIFY(canTransitionTaskState(TaskState::Commission, TaskState::RuntimeStarting));
        QVERIFY(canTransitionTaskState(TaskState::Ready, TaskState::RunningCycle));
        QVERIFY(canTransitionTaskState(TaskState::Recovering, TaskState::Ready));
        QVERIFY(canTransitionTaskState(TaskState::Faulted, TaskState::Stopping));

        QVERIFY(!canTransitionTaskState(TaskState::Idle, TaskState::Ready));
        QVERIFY(!canTransitionTaskState(TaskState::Commission, TaskState::Recovering));
        QVERIFY(!canTransitionTaskState(TaskState::RunningCycle, TaskState::Commission));
        QVERIFY(!canTransitionTaskState(TaskState::Faulted, TaskState::Ready));
    }

    void test_task_localization_state_machine_rejects_invalid_transition()
    {
        TaskLocalizationProbe task(QStringLiteral("Task State Probe"));
        QCOMPARE(task.taskState(), TaskState::Idle);
        QVERIFY(!task.transitionTaskState(TaskState::Ready,
                                          QStringLiteral("invalid test jump")));
        QCOMPARE(task.taskState(), TaskState::Idle);
    }

    void test_task_localization_state_machine_follows_commission_lifecycle()
    {
        TaskLocalizationProbe task(QStringLiteral("Task Commission Probe"));
        QCOMPARE(task.taskState(), TaskState::Idle);

        task.beginCommission();
        QCOMPARE(task.taskState(), TaskState::Commission);

        task.endCommission();
        QCOMPARE(task.taskState(), TaskState::Idle);
    }

    void test_task_localization_runtime_signals_drive_recovering_and_faulted_states()
    {
        TaskLocalizationProbe task(QStringLiteral("Task Runtime Probe"));
        QVERIFY(task.transitionTaskState(TaskState::CommissionStarting,
                                         QStringLiteral("test setup")));
        QVERIFY(task.transitionTaskState(TaskState::Commission,
                                         QStringLiteral("test setup")));
        QVERIFY(task.transitionTaskState(TaskState::RuntimeStarting,
                                         QStringLiteral("test setup")));
        QVERIFY(task.transitionTaskState(TaskState::Ready,
                                         QStringLiteral("test setup")));

        auto *controller = task.findChild<LocalizationRuntimeController *>();
        QVERIFY(controller != nullptr);

        QVERIFY(QMetaObject::invokeMethod(controller,
                                          "runtimeRecovering",
                                          Qt::DirectConnection,
                                          Q_ARG(QString, QStringLiteral("recovering"))));
        QCOMPARE(task.taskState(), TaskState::Recovering);

        QVERIFY(QMetaObject::invokeMethod(controller,
                                          "runtimeReady",
                                          Qt::DirectConnection,
                                          Q_ARG(QString, QStringLiteral("ready"))));
        QCOMPARE(task.taskState(), TaskState::Ready);

        QVERIFY(QMetaObject::invokeMethod(controller,
                                          "runtimeFault",
                                          Qt::DirectConnection,
                                          Q_ARG(QString, QStringLiteral("fault"))));
        QCOMPARE(task.taskState(), TaskState::Faulted);

        task.stopAll();
        QCOMPARE(task.taskState(), TaskState::Idle);
    }
};

QTEST_MAIN(ArchitectureContractTest)

#include "main.moc"
