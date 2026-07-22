#include "NpcVehicleAI.h"

#include <AzCore/Math/MathUtils.h>
#include <algorithm>
#include <cmath>

namespace LDMChronoVehicle
{
    AiDriverOutputs NpcVehicleAI::UpdateAiDriver(const AZ::Transform& aiVehiclePose, double currentSpeedMetersPerSec,
        const AZStd::vector<AZ::Vector3>& trackWaypoints, size_t& currentWaypointIndex,
        const AZ::Transform& playerVehiclePose, bool playerIsAlive, double simulationTimeSeconds)
    {
        AiDriverOutputs outputs;

        if (trackWaypoints.empty())
        {
            outputs.m_throttle = 0.3;
            return outputs;
        }

        // Current position and forward orientation
        const AZ::Vector3 currentPos = aiVehiclePose.GetTranslation();
        const AZ::Vector3 forward = aiVehiclePose.GetBasisX().GetNormalized();

        // Check distance to active waypoint
        const AZ::Vector3 targetWaypoint = trackWaypoints[currentWaypointIndex % trackWaypoints.size()];
        AZ::Vector3 toTarget = targetWaypoint - currentPos;
        toTarget.SetZ(0.0f); // 2D race tracking

        float distToTarget = toTarget.GetLength();
        if (distToTarget < 12.0f)
        {
            // Advance to next waypoint node
            currentWaypointIndex = (currentWaypointIndex + 1) % trackWaypoints.size();
        }

        // Pure pursuit steering
        AZ::Vector3 localTargetDir = aiVehiclePose.GetInverse().TransformVector(toTarget).GetNormalized();
        float angleToTargetRad = std::atan2(localTargetDir.GetY(), localTargetDir.GetX());
        outputs.m_steering = std::clamp(static_cast<double>(angleToTargetRad * 1.5f), -1.0, 1.0);

        // Speed management: target speed ~22 m/s (~80 km/h) on straights, slow down for sharp turns
        double targetSpeed = 22.0 - std::abs(outputs.m_steering) * 12.0;
        if (currentSpeedMetersPerSec < targetSpeed)
        {
            outputs.m_throttle = std::clamp((targetSpeed - currentSpeedMetersPerSec) * 0.15, 0.2, 0.85);
            outputs.m_braking = 0.0;
        }
        else
        {
            outputs.m_throttle = 0.0;
            outputs.m_braking = std::clamp((currentSpeedMetersPerSec - targetSpeed) * 0.1, 0.0, 0.6);
        }

        // AI Targeting & Burst Firing logic against player vehicle
        if (playerIsAlive)
        {
            AZ::Vector3 toPlayer = playerVehiclePose.GetTranslation() - currentPos;
            float distToPlayer = toPlayer.GetLength();

            if (distToPlayer < 75.0f) // Within 75m engagement range
            {
                AZ::Vector3 localPlayerDir = aiVehiclePose.GetInverse().TransformVector(toPlayer).GetNormalized();
                float angleToPlayerRad = std::atan2(localPlayerDir.GetY(), localPlayerDir.GetX());

                // Target within 25 degree forward cone
                if (std::abs(angleToPlayerRad) < 0.44f)
                {
                    // Fire 5-round burst every 1.2s
                    if (simulationTimeSeconds - m_lastFireTimeSeconds > 1.2)
                    {
                        outputs.m_fireRifles = true;
                        m_lastFireTimeSeconds = simulationTimeSeconds;
                    }
                }
            }
        }

        return outputs;
    }
} // namespace LDMChronoVehicle
