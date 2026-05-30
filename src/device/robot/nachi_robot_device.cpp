#include "nachi_robot_device.h"

namespace vc::device {

NachiRobotDevice::NachiRobotDevice(QString id, QString name, QObject* parent)
    : RobotDevice(id, name, parent) {
    setDeviceConfig(&m_cfg);
}

bool NachiRobotDevice::deviceConnect() {
    // Not implemented yet — vendor integration pending.
    return false;
}

bool NachiRobotDevice::deviceDisconnect() {
    return true;
}

bool NachiRobotDevice::isDeviceConnected() const {
    return m_connected;
}

bool NachiRobotDevice::pushRequest(IRequest *request) {
    Q_UNUSED(request);
    return false;
}

QStringList NachiRobotDevice::getAvailableBits() {
    return {};
}

QStringList NachiRobotDevice::getAvailableWords() {
    return {};
}

void NachiRobotDevice::deviceTerminate() {
    deviceDisconnect();
}

} // namespace vc::device
