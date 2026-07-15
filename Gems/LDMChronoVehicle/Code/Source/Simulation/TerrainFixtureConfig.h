#pragma once

#include <AzCore/base.h>

namespace LDMChronoVehicle
{
    // Single versioned source for the T0 shared terrain (ADR 0002). The
    // Chrono contact patch and the O3DE-rendered ground must both be built
    // from these values; bump Version whenever any value changes.
    namespace TerrainFixtureConfig
    {
        inline constexpr AZ::u32 Version = 1;
        inline constexpr double LengthMeters = 200.0;
        inline constexpr double WidthMeters = 200.0;
        inline constexpr double SurfaceZMeters = 0.0;
        inline constexpr double ThicknessMeters = 1.0;
        inline constexpr double Friction = 0.9;
        inline constexpr double Restitution = 0.01;
    } // namespace TerrainFixtureConfig

    // Fingerprint of the versioned terrain values. Every consumer logs it so
    // captured evidence proves both engines used identical terrain data.
    AZ::u32 CalculateTerrainFixtureChecksum();
} // namespace LDMChronoVehicle
