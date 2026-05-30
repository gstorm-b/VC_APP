#ifndef NACHI_ROBOT_DEVICE_H
#define NACHI_ROBOT_DEVICE_H

#include "device/robot/robot_device.h"
#include "device/robot/nachi_robot_config.h"

namespace vc::device {

// Minimum concrete robot device — Nachi vendor stub. See KawasakiRobotDevice
// for the equivalent pattern.
class NachiRobotDevice : public RobotDevice {
    Q_OBJECT

public:
    explicit NachiRobotDevice(QString id, QString name, QObject* parent = nullptr);
    ~NachiRobotDevice() override = default;

    RobotType robotType() const override { return RobotType::Nachi; }

    bool deviceConnect() override;
    bool deviceDisconnect() override;
    bool isDeviceConnected() const override;

    bool pushRequest(IRequest *request) override;

    QStringList getAvailableBits() override;
    QStringList getAvailableWords() override;

public slots:
    void deviceTerminate() override;

private:
    NachiRobotCfg m_cfg;
    bool m_connected{false};
};

} // namespace vc::device

#endif // NACHI_ROBOT_DEVICE_H
