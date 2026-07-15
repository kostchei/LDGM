#include "CoordinateConversion.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/MathUtils.h>

namespace LDMChronoVehicle
{
    namespace
    {
        // AZ math stores single-precision components; tolerances below are
        // sized for float round-trips, not double precision.
        constexpr float UnitLengthTolerance = 1e-4f;
        constexpr float UnitScaleTolerance = 1e-4f;
    } // namespace

    chrono::ChVector3d ToChronoVector(const AZ::Vector3& value)
    {
        AZ_Assert(value.IsFinite(), "Vectors crossing the Chrono boundary must be finite.");
        return chrono::ChVector3d(
            static_cast<double>(value.GetX()),
            static_cast<double>(value.GetY()),
            static_cast<double>(value.GetZ()));
    }

    AZ::Vector3 ToAzVector(const chrono::ChVector3d& value)
    {
        const AZ::Vector3 result(
            static_cast<float>(value.x()),
            static_cast<float>(value.y()),
            static_cast<float>(value.z()));
        AZ_Assert(result.IsFinite(), "Vectors leaving the Chrono boundary must be finite.");
        return result;
    }

    chrono::ChQuaterniond ToChronoQuaternion(const AZ::Quaternion& value)
    {
        AZ_Assert(value.IsFinite() && AZ::IsClose(value.GetLength(), 1.0f, UnitLengthTolerance),
            "Quaternions crossing the Chrono boundary must be finite and unit length.");
        chrono::ChQuaterniond result(
            static_cast<double>(value.GetW()),
            static_cast<double>(value.GetX()),
            static_cast<double>(value.GetY()),
            static_cast<double>(value.GetZ()));
        result.Normalize();
        return result;
    }

    AZ::Quaternion ToAzQuaternion(const chrono::ChQuaterniond& value)
    {
        const AZ::Quaternion result(
            static_cast<float>(value.e1()),
            static_cast<float>(value.e2()),
            static_cast<float>(value.e3()),
            static_cast<float>(value.e0()));
        AZ_Assert(result.IsFinite() && AZ::IsClose(result.GetLength(), 1.0f, UnitLengthTolerance),
            "Quaternions leaving the Chrono boundary must be finite and unit length.");
        return result.GetNormalized();
    }

    chrono::ChCoordsysd ToChronoPose(const AZ::Transform& value)
    {
        AZ_Assert(AZ::IsClose(value.GetUniformScale(), 1.0f, UnitScaleTolerance),
            "Transforms crossing the Chrono boundary must carry unit scale (got %f).",
            static_cast<double>(value.GetUniformScale()));
        return chrono::ChCoordsysd(
            ToChronoVector(value.GetTranslation()),
            ToChronoQuaternion(value.GetRotation()));
    }

    AZ::Transform ToAzPose(const chrono::ChCoordsysd& value)
    {
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            ToAzQuaternion(value.rot), ToAzVector(value.pos));
    }
} // namespace LDMChronoVehicle
