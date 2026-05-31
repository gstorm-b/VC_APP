#ifndef LOCALIZATION_FAULT_CODE_H
#define LOCALIZATION_FAULT_CODE_H

#include <QString>

namespace vc::model {

enum class LocalizationFaultCode : int {
    None = 0,
    CameraLost = 100,
    CameraConnectFailed = 101,
    CameraGrabTimeout = 102,
    VisionOutputLost = 200,
    VisionOutputSendFailed = 201,
    PlcLost = 300,
    PatternInvalid = 400,
    CalibrationInvalid = 401,
    InternalError = 500,
};

inline QString localizationFaultCodeName(LocalizationFaultCode code)
{
    switch (code) {
    case LocalizationFaultCode::None:
        return QStringLiteral("None");
    case LocalizationFaultCode::CameraLost:
        return QStringLiteral("CameraLost");
    case LocalizationFaultCode::CameraConnectFailed:
        return QStringLiteral("CameraConnectFailed");
    case LocalizationFaultCode::CameraGrabTimeout:
        return QStringLiteral("CameraGrabTimeout");
    case LocalizationFaultCode::VisionOutputLost:
        return QStringLiteral("VisionOutputLost");
    case LocalizationFaultCode::VisionOutputSendFailed:
        return QStringLiteral("VisionOutputSendFailed");
    case LocalizationFaultCode::PlcLost:
        return QStringLiteral("PlcLost");
    case LocalizationFaultCode::PatternInvalid:
        return QStringLiteral("PatternInvalid");
    case LocalizationFaultCode::CalibrationInvalid:
        return QStringLiteral("CalibrationInvalid");
    case LocalizationFaultCode::InternalError:
        return QStringLiteral("InternalError");
    }

    return QStringLiteral("Unknown");
}

inline int localizationFaultCodeValue(LocalizationFaultCode code)
{
    return static_cast<int>(code);
}

} // namespace vc::model

Q_DECLARE_METATYPE(vc::model::LocalizationFaultCode)

#endif // LOCALIZATION_FAULT_CODE_H
