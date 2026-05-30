#ifndef KAWASAKI_ROBOT_DEVICE_H
#define KAWASAKI_ROBOT_DEVICE_H

#include "device/robot/robot_device.h"
#include "device/robot/kawasaki_robot_config.h"

namespace vc::device {

// Minimum concrete robot device. Implements only what IDevice requires;
// all calls are stubs returning sensible defaults until the vendor
// integration starts.
class KawasakiRobotDevice : public RobotDevice {
    Q_OBJECT

public:
    explicit KawasakiRobotDevice(QString id, QString name, QObject* parent = nullptr);
    ~KawasakiRobotDevice() override = default;

    RobotType robotType() const override { return RobotType::Kawasaki; }

    bool deviceConnect() override;
    bool deviceDisconnect() override;
    bool isDeviceConnected() const override;

    bool pushRequest(IRequest *request) override;

    QStringList getAvailableBits() override;
    QStringList getAvailableWords() override;

public slots:
    void deviceTerminate() override;

private:
    KawasakiRobotCfg m_cfg;
    bool m_connected{false};
};

} // namespace vc::device

#endif // KAWASAKI_ROBOT_DEVICE_H
