#pragma once

#include <AzCore/base.h>

namespace LDMChronoVehicle
{
    using VehicleId = AZ::u64;

    inline constexpr VehicleId InvalidVehicleId = 0;
    inline constexpr AZ::u32 MaxActiveVehicles = 30;

    enum class VehicleReservationResult
    {
        Reserved,
        AlreadyReserved,
        CapacityReached,
        InvalidId,
        NotAuthoritative
    };

    struct SimulationTelemetry
    {
        bool m_isAuthoritative = false;
        AZ::u32 m_activeVehicleCount = 0;
        AZ::u32 m_lastFrameStepCount = 0;
        AZ::u64 m_totalStepCount = 0;
        double m_fixedStepSeconds = 0.0;
        double m_lastStepWallTimeMs = 0.0;
        double m_totalDroppedSimulationSeconds = 0.0;
        double m_accumulatorSeconds = 0.0;
    };
} // namespace LDMChronoVehicle
