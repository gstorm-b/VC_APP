#include <RobotKinematics/Core/Units.h>

#include <cmath>

namespace RobotKinematics::units {

namespace {
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr double kMetersPerMillimeter = 0.001;
constexpr double kDegreesPerRadian = 180.0 / kPi;
constexpr double kRadiansPerDegree = kPi / 180.0;
}

double mm(double value_mm)
{
    return value_mm * kMetersPerMillimeter;
}

double deg(double value_deg)
{
    return value_deg * kRadiansPerDegree;
}

double toMm(double value_m)
{
    return value_m / kMetersPerMillimeter;
}

double toDeg(double value_rad)
{
    return value_rad * kDegreesPerRadian;
}

} // namespace RobotKinematics::units
