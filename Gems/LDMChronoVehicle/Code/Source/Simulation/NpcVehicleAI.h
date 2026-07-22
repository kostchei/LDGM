#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace LDMChronoVehicle
{
    struct AiDriverOutputs
    {
        double m_steering = 0.0;
        double m_throttle = 0.0;
        double m_braking = 0.0;
        bool m_fireRifles = false;
    };

    class NpcVehicleAI final
    {
    public:
        NpcVehicleAI() = default;
        ~NpcVehicleAI() = default;

        // Updates AI steering, throttle, braking, and target engagement
        AiDriverOutputs UpdateAiDriver(const AZ::Transform& aiVehiclePose, double currentSpeedMetersPerSec,
            const AZStd::vector<AZ::Vector3>& trackWaypoints, size_t& currentWaypointIndex,
            const AZ::Transform& playerVehiclePose, bool playerIsAlive, double simulationTimeSeconds);

    private:
        double m_lastFireTimeSeconds = 0.0;
    };
} // namespace LDMChronoVehicle
