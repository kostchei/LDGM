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
    struct LiveVehicleInputs
    {
        double m_steering = 0.0;
        double m_throttle = 0.0;
        double m_braking = 0.0;
        bool m_handbrake = false;
        int m_driveMode = 1; // -1 = Reverse, 0 = Neutral, 1 = Forward
        bool m_engineStarted = true;
        bool m_fireLeft = false;
        bool m_fireRight = false;
    };

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

        void SetLiveInputs(const LiveVehicleInputs& inputs);
        LiveVehicleInputs GetLiveInputs() const;

        // Rifle APIs
        AZ::u32 GetLeftRifleAmmo() const;
        AZ::u32 GetRightRifleAmmo() const;
        void SetLeftRifleAmmo(AZ::u32 ammo);
        void SetRightRifleAmmo(AZ::u32 ammo);

        // Damage & Repair APIs
        float GetCondition() const;
        void SetCondition(float condition);
        AZ::u32 GetConditionState() const; // 0 = Healthy, 1 = Damaged, 2 = Critical, 3 = Destroyed
        float GetZoneHealth(int zoneId) const;
        void SetZoneHealth(int zoneId, float health);
        void ApplyDamage(float amount);
        void RepairZone(int zoneId);
        void RepairVehicle();

    private:
        double m_fixedStepSeconds = 0.0;
        chrono::vehicle::RigidTerrain& m_terrain;
        VehicleFixtureConfig m_config;
        std::unique_ptr<chrono::vehicle::hmmwv::HMMWV_Full> m_vehicle;

        LiveVehicleInputs m_liveInputs;
        bool m_useLiveInputs = false;

        // Combat/Damage States
        AZ::u32 m_leftAmmo = 50;
        AZ::u32 m_rightAmmo = 50;
        double m_leftCooldown = 0.0;
        double m_rightCooldown = 0.0;
        float m_zoneHealth[5] = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
    };
} // namespace LDMChronoVehicle
