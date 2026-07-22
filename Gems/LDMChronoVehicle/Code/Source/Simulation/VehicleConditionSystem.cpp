#include "VehicleConditionSystem.h"

#include <algorithm>

namespace LDMChronoVehicle
{
    VehicleConditionState VehicleConditionSystem::EvaluateState(float healthRatio)
    {
        if (healthRatio <= 0.0f)
        {
            return VehicleConditionState::Destroyed;
        }
        else if (healthRatio <= 0.30f)
        {
            return VehicleConditionState::Critical;
        }
        else if (healthRatio <= 0.65f)
        {
            return VehicleConditionState::Damaged;
        }
        return VehicleConditionState::Healthy;
    }

    DamageModifierResult VehicleConditionSystem::CalculatePerformanceModifiers(float healthRatio)
    {
        DamageModifierResult result;
        float clampedRatio = std::clamp(healthRatio, 0.0f, 1.0f);

        if (clampedRatio <= 0.0f)
        {
            result.m_torqueMultiplier = 0.0;
            result.m_frictionMultiplier = 0.2;
            result.m_maxSpeedMultiplier = 0.0;
            result.m_isDestroyed = true;
            return result;
        }

        // Hit & zone damage degrades torque and friction:
        // Healthy (100%): 100% torque, 100% friction
        // Damaged (50%): 75% torque, 80% friction
        // Critical (10%): 40% torque, 60% friction
        result.m_torqueMultiplier = 0.40 + 0.60 * static_cast<double>(clampedRatio);
        result.m_frictionMultiplier = 0.60 + 0.40 * static_cast<double>(clampedRatio);
        result.m_maxSpeedMultiplier = 0.50 + 0.50 * static_cast<double>(clampedRatio);
        result.m_isDestroyed = false;

        return result;
    }

    float VehicleConditionSystem::CalculateRammingDamage(float relativeVelMetersPerSec)
    {
        // Ramming below 4 m/s (14.4 km/h) causes negligible bumper contact
        if (relativeVelMetersPerSec < 4.0f)
        {
            return 0.0f;
        }

        // Structural damage scales with impact speed delta V
        // 10 m/s (36 km/h) impact -> 20% zone damage
        // 20 m/s (72 km/h) impact -> 50% zone damage
        float damage = (relativeVelMetersPerSec - 4.0f) * 0.035f;
        return std::clamp(damage, 0.0f, 0.50f);
    }
} // namespace LDMChronoVehicle
