#pragma once

#include <AzCore/base.h>

#include <memory>

namespace chrono
{
    class ChSystem;

    namespace vehicle
    {
        class RigidTerrain;
    } // namespace vehicle
} // namespace chrono

namespace LDMChronoVehicle
{
    // Single versioned source for the T0 shared terrain (ADR 0002). The
    // Chrono contact patch and the O3DE-rendered ground must both be built
    // from these values; bump Version whenever any value changes.
    namespace TerrainFixtureConfig
    {
        inline constexpr AZ::u32 Version = 2;
        inline constexpr double LengthMeters = 200.0;
        inline constexpr double WidthMeters = 200.0;
        inline constexpr double SurfaceZMeters = 0.0;
        inline constexpr double ThicknessMeters = 1.0;

        inline constexpr int NumLanes = 5;
        inline constexpr double LaneWidthMeters = 40.0;

        inline constexpr double HardRoadFriction = 0.9;
        inline constexpr double BrokenRoadFriction = 0.7;
        inline constexpr double GravelFriction = 0.5;
        inline constexpr double FirmSandFriction = 0.4;
        inline constexpr double DeepSandFriction = 0.2;

        inline constexpr double DefaultRestitution = 0.01;

        int GetLaneIndex(double y);
        double GetLaneFriction(double y);
        const char* GetLaneName(double y);

        // The O3DE presentation uses the stock 512 m ground-plane model,
        // uniformly scaled to the same 200 m extent as the Chrono patch.
        inline constexpr double O3deGroundModelSizeMeters = 512.0;
        inline constexpr double O3deGroundUniformScale =
            LengthMeters / O3deGroundModelSizeMeters;
        inline constexpr double O3deGroundOriginXMeters = 0.0;
        inline constexpr double O3deGroundOriginYMeters = 0.0;
        inline constexpr double O3deGroundSurfaceZMeters = SurfaceZMeters;
    } // namespace TerrainFixtureConfig

    // Fingerprint of the versioned terrain values. Every consumer logs it so
    // captured evidence proves both engines used identical terrain data.
    AZ::u32 CalculateTerrainFixtureChecksum();

    struct TerrainAlignmentMeasurement
    {
        double m_originErrorMeters = 0.0;
        double m_surfaceHeightErrorMeters = 0.0;
        double m_renderedLengthErrorMeters = 0.0;
        double m_renderedWidthErrorMeters = 0.0;

        bool Passes(double toleranceMeters = 0.02) const;
    };

    // Measures the checked-in O3DE presentation configuration against the
    // authoritative Chrono terrain values. Both consumers use the constants
    // above, so this is also emitted at runtime as T0-PHYS-002 evidence.
    TerrainAlignmentMeasurement MeasureTerrainAlignment(
        double o3deOriginXMeters = TerrainFixtureConfig::O3deGroundOriginXMeters,
        double o3deOriginYMeters = TerrainFixtureConfig::O3deGroundOriginYMeters,
        double o3deSurfaceZMeters = TerrainFixtureConfig::O3deGroundSurfaceZMeters,
        double o3deUniformScale = TerrainFixtureConfig::O3deGroundUniformScale);

    class TerrainFixture final
    {
    public:
        explicit TerrainFixture(chrono::ChSystem& system);
        ~TerrainFixture();

        chrono::vehicle::RigidTerrain& GetTerrain();
        void Synchronize(double simulationTimeSeconds);
        void Advance(double fixedStepSeconds);

    private:
        std::unique_ptr<chrono::vehicle::RigidTerrain> m_terrain;
    };
} // namespace LDMChronoVehicle
