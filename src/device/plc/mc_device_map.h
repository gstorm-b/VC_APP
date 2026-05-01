#ifndef MC_DEVICE_MAP_H
#define MC_DEVICE_MAP_H

#include <vector>
#include <QByteArray>
#include <QSet>
#include <map>
// #include "device/plc/mc_protocol_device.h"

#define MC_MAXIMUM_DEVICE_AMOUNT   256

namespace vc::device {

class PlcValueMap {
public:
    virtual ~PlcValueMap() = default;
    virtual std::shared_ptr<PlcValueMap> clone() const = 0;
};

class McDeviceRange {
public:
    struct DeviceRange {
        int start;
        int end;
        int amount;
    };

    McDeviceRange();
    ~McDeviceRange();

    /**
         * @brief SubscribeDevice: Add a range of devices to the subscription list
         * @param address: start address
         * @param amount: amount of device
         * @param optimizal: optimize range or not
         */
    void SubscribeDevice(int address, int amount, bool optimizal = false);

    /**
         * @brief OptimizeRange: Optimize device range to query multiple devices in a single command
         */
    void OptimizeRange();

    /**
         * @brief IsOptimzed: retrieve optimized ranges status
         * @return return optimization state
         */
    inline const bool IsOptimzed() {
        return this->has_optimized;
    }

    void clearRanges() {
        subscribed_devices.clear();
        ranges.clear();
    }

    std::vector<int> subscribed_devices;
    std::vector<DeviceRange> ranges;

private:
    bool has_optimized;
};

class McDeviceMap : public PlcValueMap {
public:
    McDeviceMap();
    ~McDeviceMap();

    std::shared_ptr<PlcValueMap> clone() const {
        return std::make_shared<McDeviceMap>(*this);
    }

    /**
         * @brief Subscribe_deivce: Add a range of devices of the same type to the subscription list
         * @param device: device type in uppercase: 'M', 'D', 'X',...
         * @param address: start address
         * @param amount: amount of device
         * @param optimal: optimize all ranges or not
         */
    void Subscribe_deivce(char device, int address, int amount, bool optimal = true);

    /**
         * @brief OptimizeRanges: Optimize device ranges (X, Y, M, D types) to query multiple devices in a single command
         */
    void OptimizeRanges();

    /**
         * @brief GetDeviceRange: Retrieve subscription ranges of a specific device type
         * @param device: device type in uppercase: 'X', 'Y', 'M' or 'D'
         * @return ranges of a specific device type
         */
    McDeviceRange* GetDeviceRange(char device);

    void clearMap() {
        x_devices.clearRanges();
        y_devices.clearRanges();
        m_devices.clearRanges();
        d_devices.clearRanges();


        device_map_m.clear();
        device_map_d.clear();
    }

    McDeviceRange x_devices;
    McDeviceRange y_devices;
    McDeviceRange m_devices;
    McDeviceRange d_devices;

    std::map<int, quint8> device_map_m;
    std::map<int, qint16> device_map_d;
};

}; // namespace vc::device

#endif // MC_DEVICE_MAP_H
