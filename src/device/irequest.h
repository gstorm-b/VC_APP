#ifndef IREQUEST_H
#define IREQUEST_H

#include <memory>

namespace vc::device {

enum RequestType {
    Request_MC,
    Request_PLC,
    Request_VisionOutput
};

class IRequest {
public:
    virtual ~IRequest() = default;
    virtual RequestType type() const = 0;
    virtual std::shared_ptr<IRequest> clone() const = 0;
};

}

#endif // IREQUEST_H
