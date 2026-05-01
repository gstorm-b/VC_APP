#ifndef PLC_VALUE_H
#define PLC_VALUE_H

#include <vector>

namespace vc::device {

enum class PLCValueType {
    Int,
    Float,
    String
};

class ValueContainerAbstract {
public:
    virtual ~ValueContainerAbstract() = default;
    virtual PLCValueType type() const = 0;
    virtual void printAll() const = 0;
};

template<typename T>
class ValueContainer : public ValueContainerAbstract {
public:
    void add(const T& value) {
        data.push_back(value);
    }

    const std::vector<T>& values() const {
        return data;
    }

    PLCValueType type() const override {
        if constexpr (std::is_same_v<T, int>) return PLCValueType::Int;
        else if constexpr (std::is_same_v<T, float>) return PLCValueType::Float;
        else return PLCValueType::String;
    }

private:
    std::vector<T> data;
};

} // namespace vc::device

#endif // PLC_VALUE_H
