#include "kawasaki_robot_device.h"

namespace vc::device {

KawasakiRobotDevice::KawasakiRobotDevice(QString id, QString name, QObject* parent)
    : RobotDevice(id, name, parent) {
    setDeviceConfig(&m_cfg);
}

bool KawasakiRobotDevice::deviceConnect() {
    // Not implemented yet — vendor integration pending.
    return false;
}

bool KawasakiRobotDevice::deviceDisconnect() {
    return true;
}

bool KawasakiRobotDevice::isDeviceConnected() const {
    return m_connected;
}

bool KawasakiRobotDevice::pushRequest(IRequest *request) {
    Q_UNUSED(request);
    return false;
}

QStringList KawasakiRobotDevice::getAvailableBits() {
    return {};
}

QStringList KawasakiRobotDevice::getAvailableWords() {
    return {};
}

void KawasakiRobotDevice::deviceTerminate() {
    deviceDisconnect();
}

} // namespace vc::device
