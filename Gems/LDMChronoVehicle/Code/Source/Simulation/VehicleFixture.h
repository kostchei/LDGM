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
    struct VehicleFixtureConfig
    {
        double m_spawnX = 0.0;
        double m_spawnY = 0.0;
        double m_spawnYawRadians = 0.0;
        double m_targetThrottle = 0.4;
        double m_steering = 0.0;
    };

    // T0 integration fixture: one stock, fully programmatic HMMWV using a
    // caller-owned shared terrain patch, driven by a
    // deterministic scripted input schedule (settle, throttle ramp, straight
    // drive). No data files are loaded and no visualization assets are
    // created, so the fixture runs headless.
    //
    // The caller owns the Chrono system and its fixed stepping: call
    // SynchronizeAndAdvance once per fixed step, then DoStepDynamics.
    class VehicleFixture final
    {
    public:
        VehicleFixture(chrono::ChSystem& system, chrono::vehicle::RigidTerrain& terrain,
            double fixedStepSeconds, const VehicleFixtureConfig& config = {});
        ~VehicleFixture();

        void Synchronize(double simulationTimeSeconds);
        void Advance();

        AZ::Transform GetChassisPose() const;
        double GetForwardSpeedMetersPerSecond() const;

    private:
        double m_fixedStepSeconds = 0.0;
        chrono::vehicle::RigidTerrain& m_terrain;
        VehicleFixtureConfig m_config;
        std::unique_ptr<chrono::vehicle::hmmwv::HMMWV_Full> m_vehicle;
    };
} // namespace LDMChronoVehicle
