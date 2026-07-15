#include "VehicleFixture.h"

#include "CoordinateConversion.h"
#include "TerrainFixtureConfig.h"

#include <AzCore/Debug/Trace.h>

#include <chrono/physics/ChContactMaterialNSC.h>
#include <chrono/physics/ChSystem.h>
#include <chrono_models/vehicle/hmmwv/HMMWV.h>
#include <chrono_vehicle/terrain/RigidTerrain.h>

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
        constexpr double TargetThrottle = 0.4;

        // TMeasy tires substep internally; 1 ms keeps them stable under the
        // adapter's 5 ms vehicle step.
        constexpr double TireStepSeconds = 0.001;
        constexpr double SpawnHeightMeters = 1.0;

        chrono::vehicle::DriverInputs ScriptedInputsAtTime(double timeSeconds)
        {
            chrono::vehicle::DriverInputs inputs;
            inputs.m_steering = 0.0;
            inputs.m_braking = 0.0;
            inputs.m_clutch = 0.0;
            inputs.m_throttle = timeSeconds <= SettleSeconds
                ? 0.0
                : TargetThrottle * std::min((timeSeconds - SettleSeconds) / ThrottleRampSeconds, 1.0);
            return inputs;
        }
    } // namespace

    VehicleFixture::VehicleFixture(chrono::ChSystem& system, double fixedStepSeconds)
        : m_fixedStepSeconds(fixedStepSeconds)
    {
        AZ_Assert(std::isfinite(fixedStepSeconds) && fixedStepSeconds > 0.0,
            "The vehicle fixture requires a finite, positive fixed step.");

        auto material = chrono_types::make_shared<chrono::ChContactMaterialNSC>();
        material->SetFriction(static_cast<float>(TerrainFixtureConfig::Friction));
        material->SetRestitution(static_cast<float>(TerrainFixtureConfig::Restitution));

        m_terrain = std::make_unique<chrono::vehicle::RigidTerrain>(&system);
        m_terrain->AddPatch(material,
            chrono::ChCoordsysd(
                chrono::ChVector3d(0.0, 0.0, TerrainFixtureConfig::SurfaceZMeters), chrono::QUNIT),
            TerrainFixtureConfig::LengthMeters,
            TerrainFixtureConfig::WidthMeters,
            TerrainFixtureConfig::ThicknessMeters,
            false, 1.0, /*visualization*/ false);
        m_terrain->Initialize();

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
            chrono::ChVector3d(0.0, 0.0, SpawnHeightMeters), chrono::QUNIT));
        m_vehicle->Initialize();

        m_vehicle->SetChassisVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetSuspensionVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetSteeringVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetWheelVisualizationType(chrono::VisualizationType::NONE);
        m_vehicle->SetTireVisualizationType(chrono::VisualizationType::NONE);
    }

    VehicleFixture::~VehicleFixture() = default;

    void VehicleFixture::SynchronizeAndAdvance(double simulationTimeSeconds)
    {
        const chrono::vehicle::DriverInputs inputs = ScriptedInputsAtTime(simulationTimeSeconds);
        m_terrain->Synchronize(simulationTimeSeconds);
        m_vehicle->Synchronize(simulationTimeSeconds, inputs, *m_terrain);
        m_terrain->Advance(m_fixedStepSeconds);
        // The system is external, so this advances vehicle subsystems (tire
        // substeps, driveline) only; the caller performs DoStepDynamics.
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
} // namespace LDMChronoVehicle
