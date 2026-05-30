#ifndef DEVICE_CAPABILITIES_H
#define DEVICE_CAPABILITIES_H

#include <QStringList>

namespace vc::device {

class IDigitalIoProvider {
public:
    virtual ~IDigitalIoProvider() = default;
    virtual QStringList availableDigitalIoNames() const = 0;
};

class IWordIoProvider {
public:
    virtual ~IWordIoProvider() = default;
    virtual QStringList availableWordIoNames() const = 0;
};

class IPlcTagProvider : public IDigitalIoProvider, public IWordIoProvider {
public:
    ~IPlcTagProvider() override = default;
};

class IImageSourceDevice {
public:
    virtual ~IImageSourceDevice() = default;
};

class IResultOutputDevice {
public:
    virtual ~IResultOutputDevice() = default;
};

} // namespace vc::device

#endif // DEVICE_CAPABILITIES_H
