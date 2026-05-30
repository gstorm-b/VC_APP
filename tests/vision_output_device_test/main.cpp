#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QHostAddress>
#include <QSignalSpy>
#include <QTcpSocket>
#include <QtTest/QtTest>

#include "device/output_device/vision_output_device.h"
#include "device/output_device/vision_tcpip_config.h"
#include "device/output_device/vision_tcpip_device.h"
#include "device/output_device/vision_output_request.h"

using namespace vc::device;

namespace {

constexpr int   kMainPort  = 25101;
constexpr int   kHbPort    = 25102;
constexpr int   kHbInterval = 200;
constexpr int   kHbTimeout  = 500;
constexpr char  kListen[]  = "127.0.0.1";

VisionTcpipDeviceCfg makeCfg() {
    VisionTcpipDeviceCfg cfg;
    cfg.m_listenAddress       = kListen;
    cfg.m_mainPort            = kMainPort;
    cfg.m_heartbeatPort       = kHbPort;
    cfg.m_heartbeatIntervalMs = kHbInterval;
    cfg.m_heartbeatTimeoutMs  = kHbTimeout;
    return cfg;
}

bool waitConnected(QTcpSocket *sock, int ms = 1000) {
    return sock->waitForConnected(ms);
}

// Vừa pump global event loop (để server slot trong cùng thread chạy),
// vừa chờ socket client có data.
bool waitReadable(QTcpSocket *sock, int ms = 1500) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) {
        if (sock->bytesAvailable() > 0) return true;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        if (sock->bytesAvailable() > 0) return true;
        sock->waitForReadyRead(20);
    }
    return sock->bytesAvailable() > 0;
}

} // namespace

class VisionOutputDeviceTest : public QObject {
    Q_OBJECT

private slots:

    void initTestCase() {
        // Đảm bảo gặp QSignalSpy thì biết Qt event loop sẽ chạy.
    }

    void test_config_clone_and_json() {
        VisionTcpipDeviceCfg cfg = makeCfg();
        QJsonObject obj = cfg.toJson();

        VisionTcpipDeviceCfg restored;
        QVERIFY(restored.fromJson(obj));
        QCOMPARE(restored.m_listenAddress,       cfg.m_listenAddress);
        QCOMPARE(restored.m_mainPort,            cfg.m_mainPort);
        QCOMPARE(restored.m_heartbeatPort,       cfg.m_heartbeatPort);
        QCOMPARE(restored.m_heartbeatIntervalMs, cfg.m_heartbeatIntervalMs);
        QCOMPARE(restored.m_heartbeatTimeoutMs,  cfg.m_heartbeatTimeoutMs);

        std::unique_ptr<IDeviceCfg> cloned(cfg.clone());
        QVERIFY(cloned != nullptr);
        QCOMPARE(cloned->deviceType(), DeviceType::VisionOutput);
    }

    void test_request_payload_format() {
        VisionOutputRequest req(QVector<VisionOutputPosition>{
            { 10.0, 20.0, 0.0,  90.0 },
            { 15.5, 25.5, 1.25, 45.0 }
        });

        QByteArray payload = req.buildPayload();
        QVERIFY(payload.startsWith("2,"));
        QVERIFY(payload.endsWith(';'));

        // Theo spec format: "{count},x,y,z,r,x,y,z,r;"
        // Tổng số dấu ',' = (4 trục - 1) cho mỗi pos + 1 ngăn cách count/pos
        // + (N-1) ngăn cách giữa các pos = N*3 + 1 + (N-1) = 4*N
        const int N = 2;
        QCOMPARE(payload.count(','), 4 * N);

        // Bóc giá trị: bỏ ';' cuối rồi split bằng ','
        QByteArray body = payload.left(payload.size() - 1);
        QList<QByteArray> parts = body.split(',');
        QCOMPARE(parts.size(), 1 + 4 * N);
        QCOMPARE(parts.first(), QByteArray("2"));

        // Empty request -> "0;"
        VisionOutputRequest empty(QVector<VisionOutputPosition>{});
        QCOMPARE(empty.buildPayload(), QByteArray("0;"));
    }

    void test_connect_disconnect_lifecycle() {
        VisionTcpipDevice dev("dev1", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);

        QVERIFY(dev.deviceConnect());
        QVERIFY(dev.isDeviceConnected());
        QVERIFY(dev.deviceDisconnect());
        QVERIFY(!dev.isDeviceConnected());
    }

    void test_heartbeat_happy_path() {
        VisionTcpipDevice dev("dev_hb", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);
        QVERIFY(dev.deviceConnect());

        QTcpSocket hb;
        hb.connectToHost(QHostAddress(kListen), cfg.m_heartbeatPort);
        QVERIFY(waitConnected(&hb));

        // Nhận probe đầu tiên
        QVERIFY(waitReadable(&hb));
        QByteArray got = hb.readAll();
        QCOMPARE(got, QByteArray("connection_check."));

        // Reply ack đúng count
        QByteArray reply = "ack,0.";
        QCOMPARE(hb.write(reply), qint64(reply.size()));
        QVERIFY(hb.waitForBytesWritten(500));

        // Đợi probe thứ 2 (sau khoảng heartbeatInterval)
        QVERIFY(waitReadable(&hb, kHbInterval * 5));
        got = hb.readAll();
        QCOMPARE(got, QByteArray("connection_check."));

        QVERIFY(dev.isDeviceConnected());

        reply = "ack,1.";
        hb.write(reply);
        hb.waitForBytesWritten(500);

        // Cho server slot onHeartbeatReadyRead chạy để parse reply.
        QElapsedTimer t; t.start();
        while (dev.expectedAckCount() != 2 && t.elapsed() < 1000) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        }
        QCOMPARE(int(dev.expectedAckCount()), 2);

        dev.deviceDisconnect();
    }

    void test_heartbeat_invalid_format_triggers_lost() {
        VisionTcpipDevice dev("dev_hb2", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);
        QVERIFY(dev.deviceConnect());

        QSignalSpy lostSpy(&dev, &VisionTcpipDevice::heartbeatLost);

        QTcpSocket hb;
        hb.connectToHost(QHostAddress(kListen), cfg.m_heartbeatPort);
        QVERIFY(waitConnected(&hb));
        QVERIFY(waitReadable(&hb));
        hb.readAll();

        // Reply sai format
        QByteArray bad = "garbage.";
        hb.write(bad);
        hb.waitForBytesWritten(500);

        QVERIFY(lostSpy.wait(2000));
        QCOMPARE(dev.connectStatus(), ConnectStatus::LostConnected);

        dev.deviceDisconnect();
    }

    void test_heartbeat_timeout_triggers_lost() {
        VisionTcpipDevice dev("dev_hb3", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);
        QVERIFY(dev.deviceConnect());

        QSignalSpy lostSpy(&dev, &VisionTcpipDevice::heartbeatLost);

        QTcpSocket hb;
        hb.connectToHost(QHostAddress(kListen), cfg.m_heartbeatPort);
        QVERIFY(waitConnected(&hb));
        QVERIFY(waitReadable(&hb));
        hb.readAll();
        // Không reply gì cả -> sau timeout sẽ lost.

        QVERIFY(lostSpy.wait(kHbTimeout + 2 * kHbInterval + 1500));
        QCOMPARE(dev.connectStatus(), ConnectStatus::LostConnected);

        dev.deviceDisconnect();
    }

    void test_push_request_sends_result_on_main_port() {
        VisionTcpipDevice dev("dev_main", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);
        QVERIFY(dev.deviceConnect());

        QTcpSocket main;
        main.connectToHost(QHostAddress(kListen), cfg.m_mainPort);
        QVERIFY(waitConnected(&main));

        // Cho event loop xử lý newConnection
        QTest::qWait(100);
        QVERIFY(dev.isMainClientConnected());

        VisionOutputRequest req(QVector<VisionOutputPosition>{
            { 1.0, 2.0, 3.0, 4.0 }
        });
        QVERIFY(dev.pushRequest(&req));

        QVERIFY(waitReadable(&main));
        QByteArray got = main.readAll();
        QVERIFY(got.startsWith("1,"));
        QVERIFY(got.endsWith(';'));

        dev.deviceDisconnect();
    }

    void test_main_port_receives_payload() {
        VisionTcpipDevice dev("dev_main2", "VO");
        VisionTcpipDeviceCfg cfg = makeCfg();
        dev.setDeviceConfig(&cfg);
        QVERIFY(dev.deviceConnect());

        QSignalSpy rxSpy(&dev, &VisionTcpipDevice::mainRequestReceived);

        QTcpSocket main;
        main.connectToHost(QHostAddress(kListen), cfg.m_mainPort);
        QVERIFY(waitConnected(&main));
        QTest::qWait(100);

        QByteArray req = "trigger,1;";
        main.write(req);
        main.waitForBytesWritten(500);

        QVERIFY(rxSpy.wait(1000));
        QCOMPARE(rxSpy.first().first().toByteArray(), req);

        dev.deviceDisconnect();
    }
};

QTEST_MAIN(VisionOutputDeviceTest)

#include "main.moc"
