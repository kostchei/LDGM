#pragma once

#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>

#include <chrono/core/ChCoordsys.h>
#include <chrono/core/ChQuaternion.h>
#include <chrono/core/ChVector3.h>

namespace LDMChronoVehicle
{
    // O3DE and Chrono's default ISO world frame are both right-handed Z-up in
    // meters (ADR 0002), so positions, linear velocities and angular
    // velocities map component-wise; only the float/double width and the
    // quaternion storage order (AZ xyzw, Chrono wxyz) differ.
    //
    // This header includes Chrono types and must stay inside the Gem's
    // private Source tree (ADR 0003).

    chrono::ChVector3d ToChronoVector(const AZ::Vector3& value);
    AZ::Vector3 ToAzVector(const chrono::ChVector3d& value);

    // Quaternions must arrive unit-length; conversion asserts and
    // re-normalizes in double precision after widening.
    chrono::ChQuaterniond ToChronoQuaternion(const AZ::Quaternion& value);
    AZ::Quaternion ToAzQuaternion(const chrono::ChQuaterniond& value);

    // Poses are rigid transforms. The O3DE transform must carry unit scale;
    // Chrono bodies have no scale concept and a scaled transform would
    // silently corrupt the simulation, so non-unit scale asserts.
    chrono::ChCoordsysd ToChronoPose(const AZ::Transform& value);
    AZ::Transform ToAzPose(const chrono::ChCoordsysd& value);
} // namespace LDMChronoVehicle
