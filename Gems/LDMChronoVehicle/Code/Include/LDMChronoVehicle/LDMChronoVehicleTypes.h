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

        // T0 vehicle/terrain fixture (authoritative role only).
        bool m_vehicleFixtureActive = false;
        AZ::u32 m_terrainChecksum = 0;
        double m_vehiclePositionX = 0.0;
        double m_vehiclePositionY = 0.0;
        double m_vehiclePositionZ = 0.0;
        double m_vehicleSpeedMetersPerSecond = 0.0;
    };
} // namespace LDMChronoVehicle
