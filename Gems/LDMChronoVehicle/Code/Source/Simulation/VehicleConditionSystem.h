#pragma once

#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Vector3.h>

namespace LDMChronoVehicle
{
    enum class VehicleConditionState : AZ::u32
    {
        Healthy = 0,
        Damaged = 1,
        Critical = 2,
        Destroyed = 3
    };

    struct DamageModifierResult
    {
        double m_torqueMultiplier = 1.0;
        double m_frictionMultiplier = 1.0;
        double m_maxSpeedMultiplier = 1.0;
        bool m_isDestroyed = false;
    };

    class VehicleConditionSystem final
    {
    public:
        VehicleConditionSystem() = default;
        ~VehicleConditionSystem() = default;

        // Evaluates condition state from overall health ratio [0.0, 1.0]
        static VehicleConditionState EvaluateState(float healthRatio);

        // Computes dynamic powertrain & friction multipliers from health
        static DamageModifierResult CalculatePerformanceModifiers(float healthRatio);

        // Calculates damage inflicted by a physical ramming collision impulse (relative velocity delta V in m/s)
        static float CalculateRammingDamage(float relativeVelMetersPerSec);
    };
} // namespace LDMChronoVehicle
