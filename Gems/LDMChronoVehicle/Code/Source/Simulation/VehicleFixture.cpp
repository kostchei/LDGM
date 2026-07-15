#include "VehicleFixture.h"

#include "CoordinateConversion.h"
#include "TerrainFixtureConfig.h"

#include <AzCore/Debug/Trace.h>

#include <chrono/physics/ChSystem.h>
#include <chrono_models/vehicle/hmmwv/HMMWV.h>
#include <chrono_vehicle/terrain/RigidTerrain.h>
#include <chrono_vehicle/ChTransmission.h>

#include <algorithm>
#include <cmath>

namespace LDMChronoVehicle
{
    namespace
    {
        // Deterministic scripted driver: settle, ramp the throttle, then
        // hold a straight-line drive. Pure function of simulation time so
        // repeated runs produce comparable transform traces.
        constexpr double SettleSeconds = 0.75;
        constexpr double ThrottleRampSeconds = 0.5;
        // TMeasy tires substep internally; 1 ms keeps them stable under the
        // adapter's 5 ms vehicle step.
        constexpr double TireStepSeconds = 0.001;
        constexpr double SpawnHeightMeters = 1.0;

        chrono::vehicle::DriverInputs ScriptedInputsAtTime(
            double timeSeconds, const VehicleFixtureConfig& config)
        {
            chrono::vehicle::DriverInputs inputs;
            inputs.m_steering = config.m_steering;
            inputs.m_braking = 0.0;
            inputs.m_clutch = 0.0;
            inputs.m_throttle = timeSeconds <= SettleSeconds
                ? 0.0
                : config.m_targetThrottle *
                    std::min((timeSeconds - SettleSeconds) / ThrottleRampSeconds, 1.0);
            return inputs;
        }
    } // namespace

    VehicleFixture::VehicleFixture(chrono::ChSystem& system,
        chrono::vehicle::RigidTerrain& terrain, double fixedStepSeconds,
        const VehicleFixtureConfig& config)
        : m_fixedStepSeconds(fixedStepSeconds)
        , m_terrain(terrain)
        , m_config(config)
    {
        AZ_Assert(std::isfinite(fixedStepSeconds) && fixedStepSeconds > 0.0,
            "The vehicle fixture requires a finite, positive fixed step.");

        m_vehicle = std::make_unique<chrono::vehicle::hmmwv::HMMWV_Full>(&system);
        m_vehicle->SetChassisFixed(false);
        m_vehicle->SetChassisCollisionType(chrono::vehicle::CollisionType::NONE);
        m_vehicle->SetEngineType(chrono::vehicle::EngineModelType::SIMPLE_MAP);
        m_vehicle->SetTransmissionType(chrono::vehicle::TransmissionModelType::AUTOMATIC_SIMPLE_MAP);
        m_vehicle->SetDriveType(chrono::vehicle::DrivelineTypeWV::AWD);
        m_vehicle->SetBrakeType(chrono::vehicle::BrakeType::SIMPLE);
        m_vehicle->SetTireType(chrono::vehicle::TireModelType::TMEASY);
        m_vehicle->SetTireStepSize(TireStepSeconds);
        m_vehicle->SetInitPosition(chrono::ChCoordsysd(
            chrono::ChVector3d(config.m_spawnX, config.m_spawnY, SpawnHeightMeters),
            chrono::QuatFromAngleZ(config.m_spawnYawRadians)));
        m_vehicle->Initialize();

        m_vehicle->SetChassisVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetSuspensionVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetSteeringVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetWheelVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetTireVisualizationType(chrono::VisualizationType::NONE);
    }

    VehicleFixture::~VehicleFixture() = default;

    void VehicleFixture::Synchronize(double simulationTimeSeconds)
    {
        if (m_useLiveInputs)
        {
            if (m_liveInputs.m_fireLeft && m_leftAmmo > 0 && simulationTimeSeconds >= m_leftCooldown)
            {
                --m_leftAmmo;
                m_leftCooldown = simulationTimeSeconds + 0.15;
                AZ_Printf("LDMChronoVehicle", "Rifle fired: left ammo_left=%u\n", m_leftAmmo);
            }
            if (m_liveInputs.m_fireRight && m_rightAmmo > 0 && simulationTimeSeconds >= m_rightCooldown)
            {
                --m_rightAmmo;
                m_rightCooldown = simulationTimeSeconds + 0.15;
                AZ_Printf("LDMChronoVehicle", "Rifle fired: right ammo_left=%u\n", m_rightAmmo);
            }
        }

        chrono::vehicle::DriverInputs inputs;
        if (m_useLiveInputs)
        {
            inputs.m_steering = m_liveInputs.m_steering;
            inputs.m_throttle = m_liveInputs.m_engineStarted ? m_liveInputs.m_throttle : 0.0;
            inputs.m_braking = m_liveInputs.m_braking;
            inputs.m_clutch = 0.0;
            if (m_liveInputs.m_handbrake)
            {
                inputs.m_braking = std::max(inputs.m_braking, 1.0);
            }

            if (m_vehicle)
            {
                auto transmission = m_vehicle->GetVehicle().GetTransmission();
                if (transmission)
                {
                    auto autoTrans = transmission->asAutomatic();
                    if (autoTrans)
                    {
                        if (m_liveInputs.m_driveMode == 1)
                        {
                            autoTrans->SetDriveMode(chrono::vehicle::ChAutomaticTransmission::DriveMode::FORWARD);
                        }
                        else if (m_liveInputs.m_driveMode == -1)
                        {
                            autoTrans->SetDriveMode(chrono::vehicle::ChAutomaticTransmission::DriveMode::REVERSE);
                        }
                        else
                        {
                            autoTrans->SetDriveMode(chrono::vehicle::ChAutomaticTransmission::DriveMode::NEUTRAL);
                        }
                    }
                }
            }
        }
        else
        {
            inputs = ScriptedInputsAtTime(simulationTimeSeconds, m_config);
        }

        const AZ::u32 conditionState = GetConditionState();

        if (conditionState == 1) // Damaged
        {
            inputs.m_throttle *= 0.75;
        }
        else if (conditionState == 2) // Critical
        {
            inputs.m_throttle *= 0.40;
            double driftFactor = 0.05 * std::sin(simulationTimeSeconds * 5.0);
            inputs.m_steering = std::clamp(inputs.m_steering + driftFactor, -1.0, 1.0);
        }
        else if (conditionState == 3) // Destroyed
        {
            inputs.m_throttle = 0.0;
            inputs.m_braking = 1.0;
        }

        m_vehicle->Synchronize(simulationTimeSeconds, inputs, m_terrain);
    }

    void VehicleFixture::Advance()
    {
        m_vehicle->Advance(m_fixedStepSeconds);
    }

    AZ::Transform VehicleFixture::GetChassisPose() const
    {
        const chrono::ChCoordsysd pose(
            m_vehicle->GetVehicle().GetPos(), m_vehicle->GetVehicle().GetRot());
        return ToAzPose(pose);
    }

    double VehicleFixture::GetForwardSpeedMetersPerSecond() const
    {
        return m_vehicle->GetVehicle().GetSpeed();
    }

    void VehicleFixture::SetLiveInputs(const LiveVehicleInputs& inputs)
    {
        m_liveInputs = inputs;
        m_useLiveInputs = true;
    }

    LiveVehicleInputs VehicleFixture::GetLiveInputs() const
    {
        return m_liveInputs;
    }

    AZ::u32 VehicleFixture::GetLeftRifleAmmo() const
    {
        return m_leftAmmo;
    }

    AZ::u32 VehicleFixture::GetRightRifleAmmo() const
    {
        return m_rightAmmo;
    }

    void VehicleFixture::SetLeftRifleAmmo(AZ::u32 ammo)
    {
        m_leftAmmo = ammo;
    }

    void VehicleFixture::SetRightRifleAmmo(AZ::u32 ammo)
    {
        m_rightAmmo = ammo;
    }

    float VehicleFixture::GetCondition() const
    {
        float total = 0.0f;
        for (int i = 0; i < 5; ++i)
        {
            total += m_zoneHealth[i];
        }
        return total / 5.0f;
    }

    void VehicleFixture::SetCondition(float condition)
    {
        float zoneVal = std::clamp(condition, 0.0f, 1.0f);
        for (int i = 0; i < 5; ++i)
        {
            m_zoneHealth[i] = zoneVal;
        }
    }

    AZ::u32 VehicleFixture::GetConditionState() const
    {
        float cond = GetCondition();
        if (cond <= 0.0f)
        {
            return 3; // Destroyed
        }
        else if (cond <= 0.3f)
        {
            return 2; // Critical
        }
        else if (cond <= 0.6f)
        {
            return 1; // Damaged
        }
        return 0; // Healthy
    }

    float VehicleFixture::GetZoneHealth(int zoneId) const
    {
        if (zoneId >= 0 && zoneId < 5)
        {
            return m_zoneHealth[zoneId];
        }
        return 0.0f;
    }

    void VehicleFixture::SetZoneHealth(int zoneId, float health)
    {
        if (zoneId >= 0 && zoneId < 5)
        {
            m_zoneHealth[zoneId] = std::clamp(health, 0.0f, 1.0f);
        }
    }

    void VehicleFixture::ApplyDamage(float amount)
    {
        float damageLeft = amount;
        for (int i = 0; i < 5 && damageLeft > 0.0f; ++i)
        {
            if (m_zoneHealth[i] > 0.0f)
            {
                float deduct = std::min(m_zoneHealth[i], damageLeft);
                m_zoneHealth[i] -= deduct;
                damageLeft -= deduct;
            }
        }
    }

    void VehicleFixture::RepairZone(int zoneId)
    {
        if (zoneId >= 0 && zoneId < 5)
        {
            m_zoneHealth[zoneId] = 1.0f;
        }
    }

    void VehicleFixture::RepairVehicle()
    {
        for (int i = 0; i < 5; ++i)
        {
            m_zoneHealth[i] = 1.0f;
        }
    }
} // namespace LDMChronoVehicle
