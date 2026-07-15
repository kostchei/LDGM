#pragma once

#include <AzCore/Math/Transform.h>

#include <memory>

namespace chrono
{
    class ChSystem;

    namespace vehicle
    {
        class RigidTerrain;

        namespace hmmwv
        {
            class HMMWV_Full;
        } // namespace hmmwv
    } // namespace vehicle
} // namespace chrono

namespace LDMChronoVehicle
{
    // T0 integration fixture: one stock, fully programmatic HMMWV on the
    // shared flat terrain patch built from TerrainFixtureConfig, driven by a
    // deterministic scripted input schedule (settle, throttle ramp, straight
    // drive). No data files are loaded and no visualization assets are
    // created, so the fixture runs headless.
    //
    // The caller owns the Chrono system and its fixed stepping: call
    // SynchronizeAndAdvance once per fixed step, then DoStepDynamics.
    class VehicleFixture final
    {
    public:
        VehicleFixture(chrono::ChSystem& system, double fixedStepSeconds);
        ~VehicleFixture();

        void SynchronizeAndAdvance(double simulationTimeSeconds);

        AZ::Transform GetChassisPose() const;
        double GetForwardSpeedMetersPerSecond() const;

    private:
        double m_fixedStepSeconds = 0.0;
        std::unique_ptr<chrono::vehicle::RigidTerrain> m_terrain;
        std::unique_ptr<chrono::vehicle::hmmwv::HMMWV_Full> m_vehicle;
    };
} // namespace LDMChronoVehicle
