#ifndef MEMORY_UTILS_H
#define MEMORY_UTILS_H

#include <QByteArray>

namespace vc::device {

inline void appendToByteArray_uint16(QByteArray &array, quint16 value, bool littleEndian = true) {
    if (littleEndian) {
        array.append(static_cast<char>(value & 0xFF));         // LSB
        array.append(static_cast<char>((value >> 8) & 0xFF));  // MSB
    } else {
        array.append(static_cast<char>((value >> 8) & 0xFF));  // MSB
        array.append(static_cast<char>(value & 0xFF));         // LSB
    }
}

inline void appendToByteArray_uint32(QByteArray &array, quint32 value, int byteCount, bool littleEndian = true) {
    if (byteCount < 1 || byteCount > 4) {
        return;
    }

    for (int i = 0; i < byteCount; ++i) {
        int shift = littleEndian ? (8 * i) : (8 * (byteCount - 1 - i));
        char byte = static_cast<char>((value >> shift) & 0xFF);
        array.append(byte);
    }
}

inline quint16 convert_uint16_FromBytes(const QByteArray& data, int index, bool littleEndian = true) {
    if (index < 0 || index + 1 >= data.size()) {
        return 0;
    }

    quint8 byte1 = static_cast<quint8>(data[index]);
    quint8 byte2 = static_cast<quint8>(data[index + 1]);

    if (littleEndian) {
        return static_cast<quint16>(byte1 | (byte2 << 8));
    } else {
        return static_cast<quint16>((byte1 << 8) | byte2);
    }
}

inline float real32ToFloat(quint32 dword) {
    float result;
    std::memcpy(&result, &dword, sizeof(float));
    return result;
}

inline quint32 floatToReal32(float value) {
    quint32 result;
    std::memcpy(&result, &value, sizeof(quint32));
    return result;
}

inline double real64ToDouble(quint64 dword) {
    double result;
    std::memcpy(&result, &dword, sizeof(result));
    return result;
}

inline quint64 doubleToReal64(double value) {
    quint64 result;
    std::memcpy(&result, &value, sizeof(result));
    return result;
}

} // namespace vc::device

#endif // MEMORY_UTILS_H
