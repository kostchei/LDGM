
#include "LDMChronoVehicleSystemComponent.h"

#if defined(AZ_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <xinput.h>
#endif

#include <Clients/CockpitCameraComponent.h>
#include <Clients/PoseSnapshots.h>
#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <Simulation/ActiveVehicleRegistry.h>
#include <Simulation/ChassisPresentationConfig.h>
#include <Simulation/FixedStepClock.h>
#include <Simulation/TerrainFixtureConfig.h>
#include <Simulation/VehicleFixture.h>

#if defined(IMGUI_ENABLED)
#include <imgui/imgui.h>
#endif

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>
#include <Atom/Feature/Utils/FrameCaptureBus.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/ComponentApplicationBus.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/string.h>
#include <AzCore/std/containers/vector.h>

#include <chrono/physics/ChBody.h>
#include <chrono/physics/ChSystemNSC.h>
#include <chrono_vehicle/ChWorldFrame.h>

#include <chrono>
#include <cmath>
#include <memory>

namespace LDMChronoVehicle
{
    namespace
    {
        constexpr double FixedStepSeconds = 0.005;
        constexpr AZ::u32 MaxCatchUpSteps = 8;
        constexpr const char* CubeModelAssetPath =
            "materialeditor/viewportmodels/cube.fbx.azmodel";
        constexpr const char* CylinderModelAssetPath =
            "materialeditor/viewportmodels/cylinder.fbx.azmodel";
        constexpr const char* ConeModelAssetPath =
            "materialeditor/viewportmodels/cone.fbx.azmodel";
        constexpr const char* TorusModelAssetPath =
            "materialeditor/viewportmodels/torus.fbx.azmodel";
        constexpr const char* GroundModelAssetPath =
            "objects/groudplane/groundplane_512x512m.fbx.azmodel";

        // Starter-technical hull rig built from processed primitive models:
        // an engine box ahead of the cabin, a panel-built cabin (so the
        // interior stays visible from the driver eye point; a single closed
        // box would be backface-culled from inside), and an open cargo tray.
        // Chassis local frame: +X forward, +Y left, Z up, origin at the
        // Chrono chassis reference. The hull floor sits at local z = -0.05.
        struct HullPartSpec
        {
            const char* m_name;
            const char* m_modelAssetPath;
            AZ::Vector3 m_position;
            AZ::Vector3 m_scale;
            float m_pitchAboutYDegrees; // model +Z tilts toward +X
            float m_rollAboutXDegrees;  // model +Z tilts toward -Y
        };

        constexpr float NoRotation = 0.0f;

        const HullPartSpec HullParts[] = {
            // Engine box: bonnet top at z = 0.65, below the driver eye line.
            { "EngineBox", CubeModelAssetPath,
              AZ::Vector3(1.55f, 0.0f, 0.3f), AZ::Vector3(1.3f, 1.8f, 0.7f),
              NoRotation, NoRotation },
            // Cabin box, panel by panel.
            { "CabinFloor", CubeModelAssetPath,
              AZ::Vector3(0.275f, 0.0f, -0.05f), AZ::Vector3(1.25f, 1.9f, 0.1f),
              NoRotation, NoRotation },
            // Front of the cabin: a low firewall with the pedal box in front
            // of it, a thin cowl rim under the windshield opening, and an
            // instrument binnacle behind the steering wheel. No full-height
            // dash slab: the space between cowl and firewall stays open so
            // the fittings read individually.
            { "CabinFirewall", CubeModelAssetPath,
              AZ::Vector3(0.85f, 0.0f, 0.3f), AZ::Vector3(0.1f, 1.9f, 0.66f),
              NoRotation, NoRotation },
            { "CabinCowl", CubeModelAssetPath,
              AZ::Vector3(0.82f, 0.0f, 0.7f), AZ::Vector3(0.16f, 1.9f, 0.14f),
              NoRotation, NoRotation },
            { "DashBinnacle", CubeModelAssetPath,
              AZ::Vector3(0.82f, -0.4f, 0.85f), AZ::Vector3(0.16f, 0.42f, 0.22f),
              NoRotation, NoRotation },
            // Gauge faces: flat cylinder discs proud of the binnacle, facing
            // the driver. Speedometer center, fuel left of it, oil pressure
            // right (right-hand drive: driver sits at y = -0.4; +Y is left).
            { "GaugeSpeedometer", CylinderModelAssetPath,
              AZ::Vector3(0.71f, -0.4f, 0.9f), AZ::Vector3(0.14f, 0.14f, 0.03f),
              90.0f, NoRotation },
            { "GaugeFuel", CylinderModelAssetPath,
              AZ::Vector3(0.71f, -0.27f, 0.87f), AZ::Vector3(0.09f, 0.09f, 0.03f),
              90.0f, NoRotation },
            { "GaugeOilPressure", CylinderModelAssetPath,
              AZ::Vector3(0.71f, -0.53f, 0.87f), AZ::Vector3(0.09f, 0.09f, 0.03f),
              90.0f, NoRotation },
            // Ammunition counter panel on the center dash, between driver and
            // passenger side, matching the HUD readout's physical home.
            { "AmmoPanel", CubeModelAssetPath,
              AZ::Vector3(0.78f, -0.05f, 0.85f), AZ::Vector3(0.06f, 0.14f, 0.09f),
              NoRotation, NoRotation },
            { "SteeringColumn", CylinderModelAssetPath,
              AZ::Vector3(0.78f, -0.4f, 0.82f), AZ::Vector3(0.05f, 0.05f, 0.2f),
              75.0f, NoRotation },
            // Floor controls: gear stick on the center tunnel (driver's left)
            // with a knob, clutch-brake-throttle pedals left to right ahead
            // of the driver's feet against the firewall.
            { "GearStick", CylinderModelAssetPath,
              AZ::Vector3(0.45f, -0.12f, 0.3f), AZ::Vector3(0.03f, 0.03f, 0.3f),
              -15.0f, NoRotation },
            { "GearKnob", CubeModelAssetPath,
              AZ::Vector3(0.41f, -0.12f, 0.46f), AZ::Vector3(0.05f, 0.05f, 0.05f),
              NoRotation, NoRotation },
            { "PedalClutch", CubeModelAssetPath,
              AZ::Vector3(0.78f, -0.28f, 0.15f), AZ::Vector3(0.03f, 0.09f, 0.14f),
              30.0f, NoRotation },
            { "PedalBrake", CubeModelAssetPath,
              AZ::Vector3(0.78f, -0.4f, 0.15f), AZ::Vector3(0.03f, 0.09f, 0.14f),
              30.0f, NoRotation },
            { "PedalThrottle", CubeModelAssetPath,
              AZ::Vector3(0.78f, -0.52f, 0.15f), AZ::Vector3(0.03f, 0.09f, 0.14f),
              30.0f, NoRotation },
            // Mirrors: interior rear-view at the top of the windshield
            // opening, exterior mirror head outside the driver (right) door.
            { "RearviewMirror", CubeModelAssetPath,
              AZ::Vector3(0.75f, 0.0f, 1.31f), AZ::Vector3(0.03f, 0.28f, 0.09f),
              NoRotation, NoRotation },
            { "SideMirrorRight", CubeModelAssetPath,
              AZ::Vector3(0.9f, -1.02f, 1.15f), AZ::Vector3(0.03f, 0.14f, 0.2f),
              NoRotation, NoRotation },
            { "CabinRoof", CubeModelAssetPath,
              AZ::Vector3(0.275f, 0.0f, 1.55f), AZ::Vector3(1.25f, 1.9f, 0.1f),
              NoRotation, NoRotation },
            { "CabinRearWall", CubeModelAssetPath,
              AZ::Vector3(-0.3f, 0.0f, 0.45f), AZ::Vector3(0.1f, 1.9f, 1.0f),
              NoRotation, NoRotation },
            { "CabinRearHeader", CubeModelAssetPath,
              AZ::Vector3(-0.3f, 0.0f, 1.5f), AZ::Vector3(0.1f, 1.9f, 0.2f),
              NoRotation, NoRotation },
            // Corner pillars: A-pillars at the windshield corners, B-pillars
            // at the rear corners. The side openings between them are the
            // door windows the driver sees when head-looking.
            { "CabinAPillarLeft", CubeModelAssetPath,
              AZ::Vector3(0.85f, 0.9f, 1.1f), AZ::Vector3(0.12f, 0.12f, 0.9f),
              NoRotation, NoRotation },
            { "CabinAPillarRight", CubeModelAssetPath,
              AZ::Vector3(0.85f, -0.9f, 1.1f), AZ::Vector3(0.12f, 0.12f, 0.9f),
              NoRotation, NoRotation },
            { "CabinBPillarLeft", CubeModelAssetPath,
              AZ::Vector3(-0.3f, 0.9f, 1.15f), AZ::Vector3(0.12f, 0.12f, 0.8f),
              NoRotation, NoRotation },
            { "CabinBPillarRight", CubeModelAssetPath,
              AZ::Vector3(-0.3f, -0.9f, 1.15f), AZ::Vector3(0.12f, 0.12f, 0.8f),
              NoRotation, NoRotation },
            { "CabinWindshieldHeader", CubeModelAssetPath,
              AZ::Vector3(0.85f, 0.0f, 1.5f), AZ::Vector3(0.12f, 1.9f, 0.15f),
              NoRotation, NoRotation },
            { "CabinDoorLeft", CubeModelAssetPath,
              AZ::Vector3(0.275f, 0.925f, 0.4f), AZ::Vector3(1.25f, 0.06f, 0.9f),
              NoRotation, NoRotation },
            { "CabinDoorRight", CubeModelAssetPath,
              AZ::Vector3(0.275f, -0.925f, 0.4f), AZ::Vector3(1.25f, 0.06f, 0.9f),
              NoRotation, NoRotation },
            { "CabinBenchSeat", CubeModelAssetPath,
              AZ::Vector3(0.0f, 0.0f, 0.3f), AZ::Vector3(0.5f, 1.7f, 0.2f),
              NoRotation, NoRotation },
            { "CabinSeatBack", CubeModelAssetPath,
              AZ::Vector3(-0.2f, 0.0f, 0.6f), AZ::Vector3(0.12f, 1.7f, 0.5f),
              NoRotation, NoRotation },
            // Steering wheel: torus lies in the model XY plane; pitch it up
            // toward the driver like a truck steering column.
            { "SteeringWheel", TorusModelAssetPath,
              AZ::Vector3(0.72f, 0.4f, 0.8f), AZ::Vector3(0.36f, 0.36f, 0.06f),
              75.0f, NoRotation },
            // Cargo tray: open box behind the cabin.
            { "CargoFloor", CubeModelAssetPath,
              AZ::Vector3(-1.25f, 0.0f, 0.0f), AZ::Vector3(1.7f, 1.9f, 0.1f),
              NoRotation, NoRotation },
            { "CargoWallLeft", CubeModelAssetPath,
              AZ::Vector3(-1.25f, 0.925f, 0.3f), AZ::Vector3(1.7f, 0.06f, 0.5f),
              NoRotation, NoRotation },
            { "CargoWallRight", CubeModelAssetPath,
              AZ::Vector3(-1.25f, -0.925f, 0.3f), AZ::Vector3(1.7f, 0.06f, 0.5f),
              NoRotation, NoRotation },
            { "CargoTailgate", CubeModelAssetPath,
              AZ::Vector3(-2.05f, 0.0f, 0.3f), AZ::Vector3(0.06f, 1.9f, 0.5f),
              NoRotation, NoRotation },
            { "CargoHeadboard", CubeModelAssetPath,
              AZ::Vector3(-0.45f, 0.0f, 0.3f), AZ::Vector3(0.06f, 1.9f, 0.5f),
              NoRotation, NoRotation },
            // Wheels: cylinder axis is model +Z; roll 90 degrees so the axle
            // runs along the chassis Y axis.
            { "WheelFrontLeft", CylinderModelAssetPath,
              AZ::Vector3(1.55f, 0.95f, -0.15f), AZ::Vector3(0.9f, 0.9f, 0.35f),
              NoRotation, 90.0f },
            { "WheelFrontRight", CylinderModelAssetPath,
              AZ::Vector3(1.55f, -0.95f, -0.15f), AZ::Vector3(0.9f, 0.9f, 0.35f),
              NoRotation, 90.0f },
            { "WheelRearLeft", CylinderModelAssetPath,
              AZ::Vector3(-1.35f, 0.95f, -0.15f), AZ::Vector3(0.9f, 0.9f, 0.35f),
              NoRotation, 90.0f },
            { "WheelRearRight", CylinderModelAssetPath,
              AZ::Vector3(-1.35f, -0.95f, -0.15f), AZ::Vector3(0.9f, 0.9f, 0.35f),
              NoRotation, 90.0f },
        };

        // Weapon visual rig relative to a shared mount offset: a pedestal
        // down to the hull, a barrel along +X, and iron sights on top.
        struct WeaponPartSpec
        {
            const char* m_name;
            const char* m_modelAssetPath;
            AZ::Vector3 m_offsetFromMount;
            AZ::Vector3 m_scale;
            float m_pitchAboutYDegrees;
        };

        const WeaponPartSpec WeaponParts[] = {
            { "Pedestal", CylinderModelAssetPath,
              AZ::Vector3(0.0f, 0.0f, -0.2f), AZ::Vector3(0.08f, 0.08f, 0.3f),
              NoRotation },
            { "Barrel", CylinderModelAssetPath,
              AZ::Vector3(0.35f, 0.0f, 0.0f), AZ::Vector3(0.07f, 0.07f, 1.1f),
              90.0f },
            { "RearSight", CubeModelAssetPath,
              AZ::Vector3(-0.15f, 0.0f, 0.06f), AZ::Vector3(0.03f, 0.08f, 0.05f),
              NoRotation },
            { "FrontSightPost", ConeModelAssetPath,
              AZ::Vector3(0.75f, 0.0f, 0.07f), AZ::Vector3(0.04f, 0.04f, 0.08f),
              NoRotation },
        };

        // The material-editor viewport primitives import as ~1 m meshes
        // (authored 100 cm across), except the torus at ~1.50 x 1.50 x 0.50 m
        // (source metadata: 149.6 x 149.6 x 49.8 cm). Part sizes above are
        // authored in meters; divide by the imported model dimensions to get
        // the render scale.
        AZ::Vector3 PartScaleFromMeters(const char* modelAssetPath, const AZ::Vector3& sizeMeters)
        {
            if (azstricmp(modelAssetPath, TorusModelAssetPath) == 0)
            {
                return AZ::Vector3(
                    sizeMeters.GetX() / 1.49579f,
                    sizeMeters.GetY() / 1.49579f,
                    sizeMeters.GetZ() / 0.49760f);
            }
            return sizeMeters;
        }

        bool IsAuthoritativeLauncher()
        {
            if (auto* console = AZ::Interface<AZ::IConsole>::Get())
            {
                bool isDedicated = false;
                if (console->GetCvarValue("sv_isDedicated", isDedicated) == AZ::GetValueResult::Success)
                {
                    return isDedicated;
                }
            }

            if (auto* settingsRegistry = AZ::SettingsRegistry::Get())
            {
                AZStd::string buildTargetName;
                if (settingsRegistry->Get(buildTargetName, AZ::SettingsRegistryMergeUtils::BuildTargetNameKey))
                {
                    return buildTargetName.find("ServerLauncher") != AZStd::string::npos;
                }
            }

            AZ_Fatal("LDMChronoVehicle",
                "Cannot determine the launcher role: neither the sv_isDedicated cvar nor the "
                "BuildTargetName settings-registry key is available. Refusing to guess.");
            AZ::Debug::Trace::Instance().Crash();
            return false; // Unreachable: Crash() does not return.
        }
    } // namespace

    struct LDMChronoVehicleSystemComponent::ChronoState
    {
        enum class Role
        {
            LifecycleSmoke, // one probe body, one step, then discarded
            Authoritative   // continuous stepping with the T0 vehicle fixture
        };

        struct VehicleEntry
        {
            VehicleId m_vehicleId = InvalidVehicleId;
            std::unique_ptr<VehicleFixture> m_fixture;
        };

        static constexpr VehicleId PlayerVehicleId = 1;
        static constexpr VehicleId EnemyVehicleId = 2;

        explicit ChronoState(Role role)
            : m_clock(FixedStepSeconds, MaxCatchUpSteps)
        {
            m_telemetry.m_isAuthoritative = true;
            m_telemetry.m_fixedStepSeconds = FixedStepSeconds;

            // O3DE and Chrono's default ISO world frame are both right-handed
            // Z-up, so poses cross the boundary without an axis permutation.
            AZ_Assert(chrono::vehicle::ChWorldFrame::IsISO(),
                "The Chrono world frame must remain the default ISO Z-up frame.");
            m_system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());

            if (role == Role::LifecycleSmoke)
            {
                m_probeBody = std::make_shared<chrono::ChBody>();
                m_probeBody->SetMass(1.0);
                m_probeBody->SetInertiaXX(chrono::ChVector3d(1.0, 1.0, 1.0));
                m_probeBody->SetPos(chrono::ChVector3d(0.0, 0.0, 2.0));
                m_system.AddBody(m_probeBody);
                return;
            }

            const VehicleReservationResult playerReservation =
                m_activeVehicles.Reserve(PlayerVehicleId);
            const VehicleReservationResult enemyReservation =
                m_activeVehicles.Reserve(EnemyVehicleId);
            AZ_Assert(playerReservation == VehicleReservationResult::Reserved &&
                enemyReservation == VehicleReservationResult::Reserved,
                "Reserving the T0 player/enemy fixtures failed (player=%d, enemy=%d).",
                static_cast<int>(playerReservation), static_cast<int>(enemyReservation));

            m_terrainFixture = std::make_unique<TerrainFixture>(m_system);

            VehicleFixtureConfig playerConfig;
            playerConfig.m_spawnY = -2.0;
            playerConfig.m_targetThrottle = 0.35;
            m_vehicles.push_back(VehicleEntry{
                PlayerVehicleId,
                std::make_unique<VehicleFixture>(m_system, m_terrainFixture->GetTerrain(),
                    FixedStepSeconds, playerConfig) });

            VehicleFixtureConfig enemyConfig;
            enemyConfig.m_spawnX = 12.0;
            enemyConfig.m_spawnY = 2.0;
            enemyConfig.m_targetThrottle = 0.30;
            enemyConfig.m_steering = -0.01;
            m_vehicles.push_back(VehicleEntry{
                EnemyVehicleId,
                std::make_unique<VehicleFixture>(m_system, m_terrainFixture->GetTerrain(),
                    FixedStepSeconds, enemyConfig) });
            m_snapshotPublisher = std::make_unique<PoseSnapshotPublisher>();

            m_telemetry.m_activeVehicleCount =
                aznumeric_cast<AZ::u32>(m_activeVehicles.GetActiveCount());
            m_telemetry.m_vehicleFixtureActive = !m_vehicles.empty();
            m_telemetry.m_terrainChecksum = CalculateTerrainFixtureChecksum();
        }

        chrono::ChSystemNSC m_system;
        std::shared_ptr<chrono::ChBody> m_probeBody;
        std::unique_ptr<TerrainFixture> m_terrainFixture;
        AZStd::vector<VehicleEntry> m_vehicles;
        std::unique_ptr<PoseSnapshotPublisher> m_snapshotPublisher;
        FixedStepClock m_clock;
        ActiveVehicleRegistry m_activeVehicles;
        SimulationTelemetry m_telemetry;
        bool m_clockPrimed = false;
        double m_nextTraceTimeSeconds = 0.0;
        double m_nextPublishSimTimeSeconds = 0.0;
    };

    AZ_COMPONENT_IMPL(LDMChronoVehicleSystemComponent, "LDMChronoVehicleSystemComponent",
        LDMChronoVehicleSystemComponentTypeId);

    void LDMChronoVehicleSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LDMChronoVehicleSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void LDMChronoVehicleSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LDMChronoVehicleService"));
    }

    void LDMChronoVehicleSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LDMChronoVehicleService"));
    }

    void LDMChronoVehicleSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void LDMChronoVehicleSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    LDMChronoVehicleSystemComponent::LDMChronoVehicleSystemComponent()
    {
        if (LDMChronoVehicleInterface::Get() == nullptr)
        {
            LDMChronoVehicleInterface::Register(this);
        }
    }

    LDMChronoVehicleSystemComponent::~LDMChronoVehicleSystemComponent()
    {
        if (LDMChronoVehicleInterface::Get() == this)
        {
            LDMChronoVehicleInterface::Unregister(this);
        }
    }

    void LDMChronoVehicleSystemComponent::Init()
    {
    }

    void LDMChronoVehicleSystemComponent::Activate()
    {
        LDMChronoVehicleRequestBus::Handler::BusConnect();

        auto smokeState = std::make_unique<ChronoState>(ChronoState::Role::LifecycleSmoke);
        smokeState->m_system.DoStepDynamics(FixedStepSeconds);

        const double height = chrono::vehicle::ChWorldFrame::Height(smokeState->m_probeBody->GetPos());
        const bool smokePassed = std::isfinite(height) &&
            std::abs(smokeState->m_system.GetChTime() - FixedStepSeconds) < 1e-12 && height < 2.0;
        AZ_Error("LDMChronoVehicle", smokePassed,
            "Chrono Core/Vehicle lifecycle smoke failed (time=%f, height=%f).",
            smokeState->m_system.GetChTime(), height);

        const bool isAuthoritative = IsAuthoritativeLauncher();
        if (smokePassed)
        {
            const double simulationTime = smokeState->m_system.GetChTime();
            AZ::TickBus::QueueFunction([simulationTime, height, isAuthoritative]()
            {
                AZ_Printf("LDMChronoVehicle",
                    "Chrono Core/Vehicle lifecycle smoke passed (time=%f, height=%f, role=%s).\n",
                    simulationTime, height, isAuthoritative ? "authoritative" : "client-linkage");
            });
        }

        if (isAuthoritative)
        {
            m_chronoState = std::make_unique<ChronoState>(ChronoState::Role::Authoritative);
            m_inputReceiver = AZStd::make_unique<VehicleInputReceiver>();
            AZ_Printf("LDMChronoVehicle",
                "T0 vehicle fixture created (terrain checksum=0x%08X, vehicles=%u/%u).\n",
                m_chronoState->m_telemetry.m_terrainChecksum,
                m_chronoState->m_telemetry.m_activeVehicleCount,
                MaxActiveVehicles);
            const TerrainAlignmentMeasurement alignment = MeasureTerrainAlignment();
            AZ_Printf("LDMChronoVehicle",
                "T0 terrain alignment: origin_err_m=%.6f surface_err_m=%.6f "
                "length_err_m=%.6f width_err_m=%.6f pass=%s\n",
                alignment.m_originErrorMeters,
                alignment.m_surfaceHeightErrorMeters,
                alignment.m_renderedLengthErrorMeters,
                alignment.m_renderedWidthErrorMeters,
                alignment.Passes() ? "true" : "false");
            AZ::TickBus::Handler::BusConnect();
        }
        else
        {
            // Client role: presentation entities are created in the game
            // entity context after the first snapshots arrive, when the
            // Atom scene and level are available.
            m_snapshotReceiver = AZStd::make_unique<PoseSnapshotReceiver>();
            m_inputPublisher = AZStd::make_unique<VehicleInputPublisher>();
            AZ::TickBus::Handler::BusConnect();
#if defined(IMGUI_ENABLED)
            ImGui::ImGuiUpdateListenerBus::Handler::BusConnect();
#endif
        }
    }

    void LDMChronoVehicleSystemComponent::Deactivate()
    {
#if defined(IMGUI_ENABLED)
        ImGui::ImGuiUpdateListenerBus::Handler::BusDisconnect();
#endif
        DestroyClientPresentation();
        m_snapshotReceiver.reset();
        m_inputPublisher.reset();
        m_inputReceiver.reset();
        AZ::TickBus::Handler::BusDisconnect();
        m_chronoState.reset();
        LDMChronoVehicleRequestBus::Handler::BusDisconnect();
    }

    VehicleReservationResult LDMChronoVehicleSystemComponent::ReserveActiveVehicle(VehicleId vehicleId)
    {
        if (!m_chronoState)
        {
            return VehicleReservationResult::NotAuthoritative;
        }

        const VehicleReservationResult result = m_chronoState->m_activeVehicles.Reserve(vehicleId);
        m_chronoState->m_telemetry.m_activeVehicleCount =
            aznumeric_cast<AZ::u32>(m_chronoState->m_activeVehicles.GetActiveCount());
        return result;
    }

    bool LDMChronoVehicleSystemComponent::ReleaseActiveVehicle(VehicleId vehicleId)
    {
        if (!m_chronoState)
        {
            return false;
        }

        const bool released = m_chronoState->m_activeVehicles.Release(vehicleId);
        m_chronoState->m_telemetry.m_activeVehicleCount =
            aznumeric_cast<AZ::u32>(m_chronoState->m_activeVehicles.GetActiveCount());
        return released;
    }

    SimulationTelemetry LDMChronoVehicleSystemComponent::GetSimulationTelemetry() const
    {
        return m_chronoState ? m_chronoState->m_telemetry : SimulationTelemetry{};
    }

    bool LDMChronoVehicleSystemComponent::GetActiveVehiclePose(
        VehicleId vehicleId, AZ::Transform& outPose) const
    {
        if (!m_chronoState || !m_chronoState->m_activeVehicles.Contains(vehicleId))
        {
            return false;
        }

        for (const ChronoState::VehicleEntry& vehicle : m_chronoState->m_vehicles)
        {
            if (vehicle.m_vehicleId == vehicleId && vehicle.m_fixture)
            {
                outPose = vehicle.m_fixture->GetChassisPose();
                return true;
            }
        }
        return false;
    }

    int LDMChronoVehicleSystemComponent::GetTickOrder()
    {
        // Step the authoritative simulation at the physics slot so proxy
        // components (TICK_ATTACHMENT) consume the fresh pose the same frame.
        return AZ::TICK_PHYSICS;
    }

    void LDMChronoVehicleSystemComponent::OnTick(float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        if (!m_chronoState)
        {
            OnClientTick();
            return;
        }

        // A system component may connect after the engine tick has already
        // started. That first callback's delta predates this simulation and
        // must not be reported as dropped authoritative time.
        if (!m_chronoState->m_clockPrimed)
        {
            m_chronoState->m_clockPrimed = true;
            AZ_Printf("LDMChronoVehicle",
                "T0 fixed-step clock primed: ignored_initial_delta_s=%.6f.\n",
                static_cast<double>(deltaTime));
            return;
        }

        // Receive and apply live client inputs on the server
        if (m_inputReceiver)
        {
            for (ChronoState::VehicleEntry& vehicle : m_chronoState->m_vehicles)
            {
                if (vehicle.m_vehicleId == ChronoState::PlayerVehicleId)
                {
                    VehicleInputPacket inputPacket;
                    if (m_inputReceiver->ReceiveLatest(vehicle.m_vehicleId, inputPacket))
                    {
                        LiveVehicleInputs inputs;
                        inputs.m_steering = inputPacket.m_steering;
                        inputs.m_throttle = inputPacket.m_throttle;
                        inputs.m_braking = inputPacket.m_braking;
                        inputs.m_handbrake = inputPacket.m_handbrake != 0;
                        inputs.m_driveMode = inputPacket.m_driveMode;
                        inputs.m_engineStarted = inputPacket.m_engineStarted != 0;
                        for (int i = 0; i < 6; ++i)
                        {
                            inputs.m_fireTriggers[i] = inputPacket.m_fireTriggers[i] != 0;
                        }
                        vehicle.m_fixture->SetLiveInputs(inputs);
                        vehicle.m_fixture->ProcessMissionCommand(inputPacket.m_missionCommand);
                    }
                }
            }
        }

        const FixedStepPlan plan = m_chronoState->m_clock.Advance(deltaTime);
        const auto stepStartedAt = std::chrono::steady_clock::now();
        for (AZ::u32 step = 0; step < plan.m_stepCount; ++step)
        {
            const double simulationTime = m_chronoState->m_system.GetChTime();
            if (m_chronoState->m_terrainFixture)
            {
                m_chronoState->m_terrainFixture->Synchronize(simulationTime);
            }
            for (ChronoState::VehicleEntry& vehicle : m_chronoState->m_vehicles)
            {
                vehicle.m_fixture->Synchronize(simulationTime);
            }
            if (m_chronoState->m_terrainFixture)
            {
                m_chronoState->m_terrainFixture->Advance(plan.m_fixedStepSeconds);
            }
            for (ChronoState::VehicleEntry& vehicle : m_chronoState->m_vehicles)
            {
                vehicle.m_fixture->Advance();
            }
            m_chronoState->m_system.DoStepDynamics(plan.m_fixedStepSeconds);
        }
        const auto stepFinishedAt = std::chrono::steady_clock::now();

        SimulationTelemetry& telemetry = m_chronoState->m_telemetry;
        telemetry.m_lastFrameStepCount = plan.m_stepCount;
        telemetry.m_totalStepCount += plan.m_stepCount;
        telemetry.m_lastStepWallTimeMs = plan.m_stepCount > 0
            ? std::chrono::duration<double, std::milli>(stepFinishedAt - stepStartedAt).count()
            : 0.0;
        telemetry.m_totalDroppedSimulationSeconds += plan.m_droppedSeconds;
        telemetry.m_accumulatorSeconds = plan.m_accumulatorSeconds;

        if (!m_chronoState->m_vehicles.empty() && plan.m_stepCount > 0)
        {
            const ChronoState::VehicleEntry& player = m_chronoState->m_vehicles.front();
            const AZ::Transform pose = player.m_fixture->GetChassisPose();
            const AZ::Vector3 position = pose.GetTranslation();
            telemetry.m_vehiclePositionX = static_cast<double>(position.GetX());
            telemetry.m_vehiclePositionY = static_cast<double>(position.GetY());
            telemetry.m_vehiclePositionZ = static_cast<double>(position.GetZ());
            telemetry.m_vehicleSpeedMetersPerSecond =
                player.m_fixture->GetForwardSpeedMetersPerSecond();

            // Transform trace for T0-PHYS-002 evidence: one line per
            // simulated second in the server log.
            const double simulationTime = m_chronoState->m_system.GetChTime();
            if (simulationTime >= m_chronoState->m_nextTraceTimeSeconds)
            {
                m_chronoState->m_nextTraceTimeSeconds = simulationTime + 1.0;
                AZ_Printf("LDMChronoVehicle",
                    "T0 transform trace: t=%.3f vehicle=%llu role=player pos=(%.3f, %.3f, %.3f) "
                    "speed=%.2f terrain=0x%08X active=%u total_steps=%llu "
                    "step_wall_ms=%.6f dropped_s=%.6f accumulator_s=%.6f\n",
                    simulationTime,
                    static_cast<unsigned long long>(player.m_vehicleId),
                    telemetry.m_vehiclePositionX,
                    telemetry.m_vehiclePositionY,
                    telemetry.m_vehiclePositionZ,
                    telemetry.m_vehicleSpeedMetersPerSecond,
                    telemetry.m_terrainChecksum,
                    telemetry.m_activeVehicleCount,
                    static_cast<unsigned long long>(telemetry.m_totalStepCount),
                    telemetry.m_lastStepWallTimeMs,
                    telemetry.m_totalDroppedSimulationSeconds,
                    telemetry.m_accumulatorSeconds);

                const ChronoState::VehicleEntry& enemy = m_chronoState->m_vehicles.back();
                const AZ::Vector3 enemyPosition =
                    enemy.m_fixture->GetChassisPose().GetTranslation();
                AZ_Printf("LDMChronoVehicle",
                    "T0 transform trace: t=%.3f vehicle=%llu role=enemy pos=(%.3f, %.3f, %.3f) "
                    "speed=%.2f terrain=0x%08X active=%u total_steps=%llu "
                    "step_wall_ms=%.6f dropped_s=%.6f accumulator_s=%.6f\n",
                    simulationTime,
                    static_cast<unsigned long long>(enemy.m_vehicleId),
                    static_cast<double>(enemyPosition.GetX()),
                    static_cast<double>(enemyPosition.GetY()),
                    static_cast<double>(enemyPosition.GetZ()),
                    enemy.m_fixture->GetForwardSpeedMetersPerSecond(),
                    telemetry.m_terrainChecksum,
                    telemetry.m_activeVehicleCount,
                    static_cast<unsigned long long>(telemetry.m_totalStepCount),
                    telemetry.m_lastStepWallTimeMs,
                    telemetry.m_totalDroppedSimulationSeconds,
                    telemetry.m_accumulatorSeconds);
            }

            // T0 deliverable: authoritative snapshots to one client.
            if (m_chronoState->m_snapshotPublisher &&
                simulationTime >= m_chronoState->m_nextPublishSimTimeSeconds)
            {
                m_chronoState->m_nextPublishSimTimeSeconds =
                    simulationTime + SnapshotConfig::PublishIntervalSeconds;
                for (const ChronoState::VehicleEntry& vehicle : m_chronoState->m_vehicles)
                {
                    AZ::u32 snapshotAmmo[6];
                    AZ::u8 snapshotEquipped[6];
                    for (int i = 0; i < 6; ++i)
                    {
                        snapshotAmmo[i] = vehicle.m_fixture->GetWeaponSlot(i).m_ammo;
                        snapshotEquipped[i] = vehicle.m_fixture->GetWeaponSlot(i).m_isEquipped ? 1 : 0;
                    }
                    m_chronoState->m_snapshotPublisher->Publish(
                        simulationTime,
                        vehicle.m_vehicleId,
                        vehicle.m_fixture->GetChassisPose(),
                        telemetry.m_terrainChecksum,
                        snapshotAmmo,
                        snapshotEquipped,
                        vehicle.m_fixture->GetConditionState(),
                        vehicle.m_fixture->GetCondition(),
                        static_cast<AZ::u32>(vehicle.m_fixture->GetMissionState()),
                        vehicle.m_fixture->GetFuelRatio());
                }
            }
        }
    }

    void LDMChronoVehicleSystemComponent::OnClientTick()
    {
        double targetSteering = 0.0;
        double targetThrottle = 0.0;
        double targetBrake = 0.0;
        bool targetHandbrake = false;
        bool gearUpPressed = false;
        bool gearDownPressed = false;
        bool engineStartPressed = false;
        bool targetFireTriggers[6] = { false, false, false, false, false, false };
        AZ::u32 targetMissionCommand = 0;

#if defined(AZ_PLATFORM_WINDOWS)
        // 1. Keyboard Mission Command Mapping
        if (GetAsyncKeyState('6') & 0x8000)
        {
            targetMissionCommand = 1;
        }
        else if (GetAsyncKeyState('7') & 0x8000)
        {
            targetMissionCommand = 2;
        }
        else if (GetAsyncKeyState('8') & 0x8000)
        {
            targetMissionCommand = 3;
        }
        else if (GetAsyncKeyState('9') & 0x8000)
        {
            targetMissionCommand = 9;
        }
        // 1. Keyboard & Mouse Firing
        if (GetAsyncKeyState('W') & 0x8000 || GetAsyncKeyState(VK_UP) & 0x8000)
        {
            targetThrottle = 1.0;
        }
        if (GetAsyncKeyState('S') & 0x8000 || GetAsyncKeyState(VK_DOWN) & 0x8000)
        {
            targetBrake = 1.0;
        }
        if (GetAsyncKeyState('A') & 0x8000 || GetAsyncKeyState(VK_LEFT) & 0x8000)
        {
            targetSteering -= 1.0;
        }
        if (GetAsyncKeyState('D') & 0x8000 || GetAsyncKeyState(VK_RIGHT) & 0x8000)
        {
            targetSteering += 1.0;
        }
        if (GetAsyncKeyState(VK_SPACE) & 0x8000)
        {
            targetHandbrake = true;
        }
        if (GetAsyncKeyState('R') & 0x8000)
        {
            gearUpPressed = true;
        }
        if (GetAsyncKeyState('F') & 0x8000)
        {
            gearDownPressed = true;
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            engineStartPressed = true;
        }
        if (GetAsyncKeyState('Q') & 0x8000 || GetAsyncKeyState(VK_LBUTTON) & 0x8000)
        {
            targetFireTriggers[0] = true;
        }
        if (GetAsyncKeyState('E') & 0x8000 || GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        {
            targetFireTriggers[1] = true;
        }
        if (GetAsyncKeyState('1') & 0x8000)
        {
            targetFireTriggers[2] = true;
        }
        if (GetAsyncKeyState('2') & 0x8000)
        {
            targetFireTriggers[3] = true;
        }
        if (GetAsyncKeyState('3') & 0x8000)
        {
            targetFireTriggers[4] = true;
        }
        if (GetAsyncKeyState('4') & 0x8000)
        {
            targetFireTriggers[5] = true;
        }

        // 2. Gamepad via XInput (dynamically loaded)
        typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);
        static XInputGetState_t s_XInputGetState = nullptr;
        static bool s_xinputLoaded = false;

        if (!s_xinputLoaded)
        {
            HMODULE hXInput = LoadLibraryA("xinput1_4.dll");
            if (!hXInput) hXInput = LoadLibraryA("xinput9_1_0.dll");
            if (!hXInput) hXInput = LoadLibraryA("xinput1_3.dll");
            if (hXInput)
            {
                s_XInputGetState = (XInputGetState_t)GetProcAddress(hXInput, "XInputGetState");
            }
            s_xinputLoaded = true;
        }

        if (s_XInputGetState)
        {
            XINPUT_STATE state;
            memset(&state, 0, sizeof(state));
            if (s_XInputGetState(0, &state) == ERROR_SUCCESS)
            {
                double lx = state.Gamepad.sThumbLX;
                if (std::abs(lx) > XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE)
                {
                    double normalizedLx = lx / (lx > 0 ? 32767.0 : 32768.0);
                    targetSteering = normalizedLx;
                }

                double rt = state.Gamepad.bRightTrigger;
                if (rt > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                {
                    targetThrottle = std::max(targetThrottle, rt / 255.0);
                }

                double lt = state.Gamepad.bLeftTrigger;
                if (lt > XINPUT_GAMEPAD_TRIGGER_THRESHOLD)
                {
                    targetBrake = std::max(targetBrake, lt / 255.0);
                }

                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_A)
                {
                    targetHandbrake = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER)
                {
                    gearUpPressed = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER)
                {
                    gearDownPressed = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_START)
                {
                    engineStartPressed = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_X)
                {
                    targetFireTriggers[0] = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
                {
                    targetFireTriggers[1] = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y)
                {
                    targetFireTriggers[2] = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_LEFT_THUMB)
                {
                    targetFireTriggers[3] = true;
                }
            }
        }
#endif

        if (gearUpPressed && !m_prevGearUpPressed)
        {
            m_clientDriveMode = std::min(1, m_clientDriveMode + 1);
        }
        m_prevGearUpPressed = gearUpPressed;

        if (gearDownPressed && !m_prevGearDownPressed)
        {
            m_clientDriveMode = std::max(-1, m_clientDriveMode - 1);
        }
        m_prevGearDownPressed = gearDownPressed;

        if (engineStartPressed && !m_prevEngineStartPressed)
        {
            m_clientEngineStarted = !m_clientEngineStarted;
        }
        m_prevEngineStartPressed = engineStartPressed;

        m_clientSteering = targetSteering;
        m_clientThrottle = targetThrottle;
        m_clientBrake = targetBrake;
        m_clientHandbrake = targetHandbrake;

        if (!m_snapshotReceiver)
        {
            return;
        }

        PoseSnapshotBatch packets;
        bool snapshotsReceived = m_snapshotReceiver->ReceiveLatest(packets);

        if (m_inputPublisher)
        {
            static double lastSimTime = 0.0;
            if (snapshotsReceived)
            {
                lastSimTime = packets.front().m_simulationTimeSeconds;
            }
            m_inputPublisher->Publish(lastSimTime, ChronoState::PlayerVehicleId,
                m_clientSteering, m_clientThrottle, m_clientBrake, m_clientHandbrake,
                m_clientDriveMode, m_clientEngineStarted,
                targetFireTriggers, targetMissionCommand);
        }

        if (!snapshotsReceived)
        {
            return;
        }

        EnsureClientTerrainPresentation();
        const bool shouldTrace =
            packets.front().m_simulationTimeSeconds >= m_nextSnapshotTraceTimeSeconds;
        if (shouldTrace)
        {
            m_nextSnapshotTraceTimeSeconds =
                packets.front().m_simulationTimeSeconds + 1.0;
        }

        for (const PoseSnapshotPacket& packet : packets)
        {
            AZ::EntityId& entityId = m_clientVehicleEntities[packet.m_vehicleId];
            if (!entityId.IsValid())
            {
                entityId = CreateClientVehiclePresentation(packet.m_vehicleId, packet);
            }
            if (!entityId.IsValid())
            {
                continue;
            }

            if (packet.m_vehicleId == ChronoState::PlayerVehicleId)
            {
                for (int slot = 0; slot < 6; ++slot)
                {
                    m_hudAmmo[slot] = packet.m_ammo[slot];
                    m_hudEquipped[slot] = packet.m_equipped[slot];
                }
                m_hudConditionState = packet.m_conditionState;
                m_hudCondition = packet.m_condition;
                m_hudFuelRatio = packet.m_fuelRatio;
            }

            const AZ::Transform pose = packet.GetPose();
            AZ::TransformBus::Event(
                entityId, &AZ::TransformBus::Events::SetWorldTM, pose);

            AZ::Transform appliedPose = AZ::Transform::CreateIdentity();
            AZ::TransformBus::EventResult(
                appliedPose, entityId, &AZ::TransformBus::Events::GetWorldTM);
            const float positionError =
                (appliedPose.GetTranslation() - pose.GetTranslation()).GetLength();
            const AZ::Quaternion rotationDelta =
                appliedPose.GetRotation().GetConjugate() * pose.GetRotation();
            const float rotationErrorDegrees = AZ::RadToDeg(
                2.0f * std::acos(AZ::GetClamp(
                    std::abs(rotationDelta.GetW()), 0.0f, 1.0f)));
            m_maxClientProxyPositionErrorMeters = AZ::GetMax(
                m_maxClientProxyPositionErrorMeters, positionError);
            m_maxClientProxyRotationErrorDegrees = AZ::GetMax(
                m_maxClientProxyRotationErrorDegrees, rotationErrorDegrees);

            if (packet.m_vehicleId == ChronoState::PlayerVehicleId)
            {
                EnsureClientCockpitCamera(entityId);
            }

            if (!shouldTrace)
            {
                continue;
            }

            const AZ::Vector3 position = pose.GetTranslation();
            // The rig root is transform-only; sample the first hull part for
            // model readiness evidence.
            AZ::Data::AssetId modelAssetId;
            const AZStd::vector<AZ::EntityId>& partEntityIds =
                m_clientVehiclePartEntities[packet.m_vehicleId];
            if (!partEntityIds.empty())
            {
                AZ::Render::MeshComponentRequestBus::EventResult(
                    modelAssetId, partEntityIds.front(),
                    &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
            }
            AZ_Printf("LDMChronoVehicle",
                "T0 snapshot trace: t=%.3f vehicle=%llu pos=(%.3f, %.3f, %.3f) "
                "received=%llu rejected=%llu terrain=0x%08X presentations=%zu\n",
                packet.m_simulationTimeSeconds,
                static_cast<unsigned long long>(packet.m_vehicleId),
                static_cast<double>(position.GetX()),
                static_cast<double>(position.GetY()),
                static_cast<double>(position.GetZ()),
                static_cast<unsigned long long>(m_snapshotReceiver->GetReceivedCount()),
                static_cast<unsigned long long>(m_snapshotReceiver->GetRejectedCount()),
                packet.m_terrainChecksum,
                m_clientVehicleEntities.size());
            AZ_Printf("LDMChronoVehicle",
                "T0 proxy trace: t=%.3f vehicle=%llu mesh_component=true model_asset=%s "
                "err_pos_m=%.6f err_rot_deg=%.6f max_err_pos_m=%.6f max_err_rot_deg=%.6f\n",
                packet.m_simulationTimeSeconds,
                static_cast<unsigned long long>(packet.m_vehicleId),
                modelAssetId.IsValid() ? "ready" : "missing",
                static_cast<double>(positionError),
                static_cast<double>(rotationErrorDegrees),
                static_cast<double>(m_maxClientProxyPositionErrorMeters),
                static_cast<double>(m_maxClientProxyRotationErrorDegrees));

            if (packet.m_vehicleId == ChronoState::PlayerVehicleId)
            {
                AZ_Printf("LDMChronoVehicle",
                    "T1 input trace: t=%.3f steering=%.3f throttle=%.3f brake=%.3f "
                    "handbrake=%s gear=%d engine=%s\n",
                    packet.m_simulationTimeSeconds,
                    m_clientSteering,
                    m_clientThrottle,
                    m_clientBrake,
                    m_clientHandbrake ? "true" : "false",
                    m_clientDriveMode,
                    m_clientEngineStarted ? "true" : "false");

                AZ_Printf("LDMChronoVehicle",
                    "T1 warning trace: t=%.3f state=%u condition=%.3f ammo_0=%u ammo_1=%u ammo_2=%u ammo_3=%u ammo_4=%u ammo_5=%u\n",
                    packet.m_simulationTimeSeconds,
                    packet.m_conditionState,
                    static_cast<double>(packet.m_condition),
                    packet.m_ammo[0],
                    packet.m_ammo[1],
                    packet.m_ammo[2],
                    packet.m_ammo[3],
                    packet.m_ammo[4],
                    packet.m_ammo[5]);

                const char* soundStateStr = "normal";
                if (packet.m_conditionState == 1) soundStateStr = "distressed_engine";
                else if (packet.m_conditionState == 2) soundStateStr = "severe_grind";
                else if (packet.m_conditionState == 3) soundStateStr = "silent_wreck";

                AZ_Printf("LDMChronoVehicle",
                    "T1 audio trace: t=%.3f sound_state=%s\n",
                    packet.m_simulationTimeSeconds,
                    soundStateStr);

                const char* smokeFireStr = "No external smoke or fire";
                if (packet.m_conditionState == 2) smokeFireStr = "Critical external marker: smoke_and_fire";
                else if (packet.m_conditionState == 3) smokeFireStr = "Destroyed external marker present";

                AZ_Printf("LDMChronoVehicle",
                    "T1 visual trace: t=%.3f marker=%s\n",
                    packet.m_simulationTimeSeconds,
                    smokeFireStr);

                AZ_Printf("LDMChronoVehicle",
                    "T3 mission trace: t=%.3f state=%u fuel=%.3f\n",
                    packet.m_simulationTimeSeconds,
                    packet.m_missionState,
                    static_cast<double>(packet.m_fuelRatio));

                // Cockpit visual evidence: capture the rendered first-person
                // view at fixed simulation times. Skipped (and logged) when
                // the null RHI is active, e.g. in headless probes.
                constexpr double CaptureTimesSeconds[] = { 6.0, 12.0 };
                if (m_cockpitScreenshotsTaken < AZ_ARRAY_SIZE(CaptureTimesSeconds) &&
                    packet.m_simulationTimeSeconds >=
                        CaptureTimesSeconds[m_cockpitScreenshotsTaken])
                {
                    ++m_cockpitScreenshotsTaken;
                    bool canCapture = false;
                    AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                        canCapture,
                        &AZ::Render::FrameCaptureRequestBus::Events::CanCapture);
                    if (canCapture)
                    {
                        const AZStd::string capturePath = AZStd::string::format(
                            "@user@/screenshots/t1_cockpit_%u.png",
                            m_cockpitScreenshotsTaken);
                        AZ::Render::FrameCaptureOutcome outcome;
                        AZ::Render::FrameCaptureRequestBus::BroadcastResult(
                            outcome,
                            &AZ::Render::FrameCaptureRequestBus::Events::CaptureScreenshot,
                            capturePath);
                        AZ_Printf("LDMChronoVehicle",
                            "T1 cockpit capture %u at t=%.3f: %s (%s)\n",
                            m_cockpitScreenshotsTaken,
                            packet.m_simulationTimeSeconds,
                            outcome.IsSuccess() ? "requested" : "failed",
                            outcome.IsSuccess()
                                ? capturePath.c_str()
                                : outcome.GetError().m_errorMessage.c_str());
                    }
                    else
                    {
                        AZ_Printf("LDMChronoVehicle",
                            "T1 cockpit capture %u at t=%.3f: unavailable "
                            "(null RHI or no frame capture support).\n",
                            m_cockpitScreenshotsTaken,
                            packet.m_simulationTimeSeconds);
                    }
                }
            }
        }
    }

    AZ::EntityId LDMChronoVehicleSystemComponent::CreateClientVehiclePresentation(
        VehicleId vehicleId, const PoseSnapshotPacket& packet)
    {
        // Rig root: transform only. Every visible part is a child entity so
        // one authoritative pose update moves the whole hull, cabin, tray,
        // wheels and weapon visuals together.
        AZ::Entity* entity = nullptr;
        const AZStd::string name = AZStd::string::format(
            vehicleId == ChronoState::PlayerVehicleId
                ? "T0PlayerVehicle_%llu"
                : "T0EnemyVehicle_%llu",
            static_cast<unsigned long long>(vehicleId));
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            entity, &AzFramework::GameEntityContextRequestBus::Events::CreateGameEntity,
            name.c_str());
        if (entity == nullptr)
        {
            AZ_Error("LDMChronoVehicle", false,
                "Unable to create client presentation entity for vehicle %llu.",
                static_cast<unsigned long long>(vehicleId));
            return AZ::EntityId();
        }

        entity->CreateComponent<AzFramework::TransformComponent>();
        // Activate through the game context so the entity is flagged
        // effectively-active; child parts inherit that flag when parented.
        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequestBus::Events::ActivateGameEntity,
            entity->GetId());
        const AZ::EntityId rootEntityId = entity->GetId();

        AZStd::vector<AZ::EntityId>& partEntities =
            m_clientVehiclePartEntities[vehicleId];

        for (const HullPartSpec& part : HullParts)
        {
            const AZ::Quaternion rotation =
                AZ::Quaternion::CreateRotationX(AZ::DegToRad(part.m_rollAboutXDegrees)) *
                AZ::Quaternion::CreateRotationY(AZ::DegToRad(part.m_pitchAboutYDegrees));
            const AZ::EntityId partId = CreateVehiclePartEntity(
                rootEntityId, part.m_name, part.m_modelAssetPath,
                AZ::Transform::CreateFromQuaternionAndTranslation(
                    rotation, part.m_position),
                part.m_scale);
            if (partId.IsValid())
            {
                partEntities.push_back(partId);
            }
        }

        AZ::u32 weaponVisualCount = 0;
        for (int slot = 0; slot < ChassisPresentationConfig::WeaponMountCount; ++slot)
        {
            if (packet.m_equipped[slot] == 0)
            {
                continue;
            }
            const AZ::Vector3& mount =
                ChassisPresentationConfig::WeaponMountOffsets[slot];
            for (const WeaponPartSpec& part : WeaponParts)
            {
                const AZStd::string partName = AZStd::string::format(
                    "Weapon%d%s", slot, part.m_name);
                const AZ::EntityId partId = CreateVehiclePartEntity(
                    rootEntityId, partName.c_str(), part.m_modelAssetPath,
                    AZ::Transform::CreateFromQuaternionAndTranslation(
                        AZ::Quaternion::CreateRotationY(
                            AZ::DegToRad(part.m_pitchAboutYDegrees)),
                        mount + part.m_offsetFromMount),
                    part.m_scale);
                if (partId.IsValid())
                {
                    partEntities.push_back(partId);
                    ++weaponVisualCount;
                }
            }
        }

        AZ_Printf("LDMChronoVehicle",
            "T1 vehicle rig created: vehicle=%llu hull_parts=%zu weapon_parts=%u\n",
            static_cast<unsigned long long>(vehicleId),
            AZ_ARRAY_SIZE(HullParts),
            weaponVisualCount);
        return rootEntityId;
    }

    AZ::EntityId LDMChronoVehicleSystemComponent::CreateVehiclePartEntity(
        AZ::EntityId parentEntityId,
        const char* partName,
        const char* modelAssetPath,
        const AZ::Transform& localTransform,
        const AZ::Vector3& partSizeMeters)
    {
        AZ::Entity* entity = nullptr;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            entity, &AzFramework::GameEntityContextRequestBus::Events::CreateGameEntity,
            partName);
        if (entity == nullptr)
        {
            AZ_Error("LDMChronoVehicle", false,
                "Unable to create vehicle part entity '%s'.", partName);
            return AZ::EntityId();
        }

        entity->CreateComponent<AzFramework::TransformComponent>();
        entity->CreateComponent<AzFramework::NonUniformScaleComponent>();
        if (entity->CreateComponent(AZ::Render::MeshComponentTypeId) == nullptr)
        {
            AZ_Error("LDMChronoVehicle", false,
                "The Atom runtime MeshComponent descriptor is unavailable for '%s'.",
                partName);
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                entity->GetId());
            return AZ::EntityId();
        }

        // Game-context entities are created effectively-inactive
        // (SetRuntimeActiveByDefault(false)); activate through the context so
        // the entity's active-state flags are set as well. A direct
        // Entity::Activate() leaves the flags cleared and the transform
        // parenting below would silently deactivate the part again.
        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequestBus::Events::ActivateGameEntity,
            entity->GetId());
        AZ::TransformBus::Event(
            entity->GetId(), &AZ::TransformBus::Events::SetParent, parentEntityId);
        AZ::TransformBus::Event(
            entity->GetId(), &AZ::TransformBus::Events::SetLocalTM, localTransform);
        AZ::NonUniformScaleRequestBus::Event(
            entity->GetId(), &AZ::NonUniformScaleRequests::SetScale,
            PartScaleFromMeters(modelAssetPath, partSizeMeters));
        AZ::Render::MeshComponentRequestBus::Event(
            entity->GetId(),
            &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetPath,
            AZStd::string(modelAssetPath));

        // The rig is invisible if a model silently fails to resolve; surface
        // both failure modes (catalog miss vs. mesh component not accepting
        // the asset) as hard errors.
        AZ::Data::AssetId resolvedAssetId;
        AZ::Data::AssetCatalogRequestBus::BroadcastResult(
            resolvedAssetId,
            &AZ::Data::AssetCatalogRequestBus::Events::GetAssetIdByPath,
            modelAssetPath, AZ::Data::AssetType::CreateNull(), false);
        AZ_Error("LDMChronoVehicle", resolvedAssetId.IsValid(),
            "Asset catalog cannot resolve part model '%s' for '%s'.",
            modelAssetPath, partName);
        AZ::Data::AssetId configuredAssetId;
        AZ::Render::MeshComponentRequestBus::EventResult(
            configuredAssetId, entity->GetId(),
            &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
        AZ_Error("LDMChronoVehicle", configuredAssetId.IsValid(),
            "Mesh component on '%s' did not accept model '%s' "
            "(catalog id %s, entity_state=%d).",
            partName, modelAssetPath,
            resolvedAssetId.IsValid() ? "valid" : "invalid",
            static_cast<int>(entity->GetState()));
        return entity->GetId();
    }

    void LDMChronoVehicleSystemComponent::EnsureClientTerrainPresentation()
    {
        if (m_clientTerrainEntityId.IsValid())
        {
            return;
        }

        AZ::Entity* entity = nullptr;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            entity, &AzFramework::GameEntityContextRequestBus::Events::CreateGameEntity,
            "T0SharedTerrainPresentation");
        if (entity == nullptr)
        {
            return;
        }

        entity->CreateComponent<AzFramework::TransformComponent>();
        if (entity->CreateComponent(AZ::Render::MeshComponentTypeId) == nullptr)
        {
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                entity->GetId());
            return;
        }

        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequestBus::Events::ActivateGameEntity,
            entity->GetId());
        AZ::Transform terrainPose = AZ::Transform::CreateTranslation(
            AZ::Vector3(0.0f, 0.0f,
                static_cast<float>(TerrainFixtureConfig::SurfaceZMeters)));
        terrainPose.MultiplyByUniformScale(
            static_cast<float>(TerrainFixtureConfig::O3deGroundUniformScale));
        AZ::TransformBus::Event(
            entity->GetId(), &AZ::TransformBus::Events::SetWorldTM, terrainPose);
        AZ::Render::MeshComponentRequestBus::Event(
            entity->GetId(),
            &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetPath,
            AZStd::string(GroundModelAssetPath));
        m_clientTerrainEntityId = entity->GetId();

        AZ::Transform appliedTerrainPose = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            appliedTerrainPose, entity->GetId(), &AZ::TransformBus::Events::GetWorldTM);
        const AZ::Vector3 appliedOrigin = appliedTerrainPose.GetTranslation();
        const TerrainAlignmentMeasurement alignment = MeasureTerrainAlignment(
            static_cast<double>(appliedOrigin.GetX()),
            static_cast<double>(appliedOrigin.GetY()),
            static_cast<double>(appliedOrigin.GetZ()),
            static_cast<double>(appliedTerrainPose.GetUniformScale()));
        AZ_Printf("LDMChronoVehicle",
            "T0 client terrain presentation: mesh_component=true scale=%.9f "
            "origin_err_m=%.6f surface_err_m=%.6f length_err_m=%.6f width_err_m=%.6f pass=%s\n",
            TerrainFixtureConfig::O3deGroundUniformScale,
            alignment.m_originErrorMeters,
            alignment.m_surfaceHeightErrorMeters,
            alignment.m_renderedLengthErrorMeters,
            alignment.m_renderedWidthErrorMeters,
            alignment.Passes() ? "true" : "false");
    }

    void LDMChronoVehicleSystemComponent::EnsureClientCockpitCamera(
        AZ::EntityId playerVehicleEntityId)
    {
        if (m_clientCameraEntityId.IsValid())
        {
            return;
        }

        AZ::Entity* entity = nullptr;
        AzFramework::GameEntityContextRequestBus::BroadcastResult(
            entity, &AzFramework::GameEntityContextRequestBus::Events::CreateGameEntity,
            "T0PlayerCockpitCamera");
        if (entity == nullptr)
        {
            return;
        }

        entity->CreateComponent<AzFramework::TransformComponent>();
        AZ::Component* cameraComponent = entity->CreateComponent(CameraComponentTypeId);
        entity->CreateComponent<CockpitCameraComponent>(playerVehicleEntityId);
        if (cameraComponent == nullptr)
        {
            AZ_Error("LDMChronoVehicle", false,
                "The O3DE runtime CameraComponent descriptor is unavailable.");
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                entity->GetId());
            return;
        }

        AzFramework::GameEntityContextRequestBus::Broadcast(
            &AzFramework::GameEntityContextRequestBus::Events::ActivateGameEntity,
            entity->GetId());
        Camera::CameraRequestBus::Event(
            entity->GetId(), &Camera::CameraRequestBus::Events::MakeActiveView);
        m_clientCameraEntityId = entity->GetId();
        AZ_Printf("LDMChronoVehicle",
            "T0 cockpit camera created: camera_component=true target_vehicle=%llu.\n",
            static_cast<unsigned long long>(ChronoState::PlayerVehicleId));

#if defined(IMGUI_ENABLED)
        // The ImGui manager only runs update listeners while its display
        // state is Visible; request it once so the combat HUD (reticle and
        // ammunition readout) renders over the cockpit view. Game input is
        // read directly from Win32, so ImGui consuming O3DE input events
        // does not affect driving or firing controls.
        if (!m_hudDisplayRequested)
        {
            m_hudDisplayRequested = true;
            // The engine's debug console auto-shows whenever ImGui is
            // visible; keep the cockpit view clear of it.
            if (auto* console = AZ::Interface<AZ::IConsole>::Get())
            {
                console->PerformCommand("bg_showDebugConsole false");
            }
            ImGui::ImGuiManagerBus::Broadcast(
                &ImGui::IImGuiManager::SetDisplayState, ImGui::DisplayState::Visible);
            AZ_Printf("LDMChronoVehicle", "T2 combat HUD requested ImGui overlay.\n");
        }
#endif
    }

    void LDMChronoVehicleSystemComponent::DestroyClientPresentation()
    {
        if (m_clientCameraEntityId.IsValid())
        {
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                m_clientCameraEntityId);
            m_clientCameraEntityId.SetInvalid();
        }
        for (const auto& [vehicleId, partEntityIds] : m_clientVehiclePartEntities)
        {
            AZ_UNUSED(vehicleId);
            for (const AZ::EntityId& partEntityId : partEntityIds)
            {
                if (partEntityId.IsValid())
                {
                    AzFramework::GameEntityContextRequestBus::Broadcast(
                        &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                        partEntityId);
                }
            }
        }
        m_clientVehiclePartEntities.clear();
        for (const auto& [vehicleId, entityId] : m_clientVehicleEntities)
        {
            AZ_UNUSED(vehicleId);
            if (entityId.IsValid())
            {
                AzFramework::GameEntityContextRequestBus::Broadcast(
                    &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                    entityId);
            }
        }
        m_clientVehicleEntities.clear();
        if (m_clientTerrainEntityId.IsValid())
        {
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                m_clientTerrainEntityId);
            m_clientTerrainEntityId.SetInvalid();
        }
    }

#if defined(IMGUI_ENABLED)
    void LDMChronoVehicleSystemComponent::OnImGuiUpdate()
    {
        // Draw only once the first-person cockpit view is live.
        if (!m_clientCameraEntityId.IsValid())
        {
            return;
        }

        const ImVec2 displaySize = ImGui::GetIO().DisplaySize;
        ImDrawList* drawList = ImGui::GetForegroundDrawList();
        const ImVec2 center(displaySize.x * 0.5f, displaySize.y * 0.5f);

        // Fixed-forward reticle: the rifles are hull-fixed, so the aim
        // reference is the vehicle forward axis at screen center (exact at
        // zero head-look yaw; the 3D iron sights remain the true reference).
        const ImU32 reticleColor = IM_COL32(255, 80, 40, 220);
        constexpr float gap = 6.0f;
        constexpr float arm = 18.0f;
        drawList->AddCircle(center, 10.0f, reticleColor, 24, 1.5f);
        drawList->AddLine(
            ImVec2(center.x - gap - arm, center.y), ImVec2(center.x - gap, center.y),
            reticleColor, 2.0f);
        drawList->AddLine(
            ImVec2(center.x + gap, center.y), ImVec2(center.x + gap + arm, center.y),
            reticleColor, 2.0f);
        drawList->AddLine(
            ImVec2(center.x, center.y - gap - arm), ImVec2(center.x, center.y - gap),
            reticleColor, 2.0f);
        drawList->AddLine(
            ImVec2(center.x, center.y + gap), ImVec2(center.x, center.y + gap + arm),
            reticleColor, 2.0f);

        // 3D Cockpit Wayfinder Indicator (Directional Arrow to Next Gate)
        const ImU32 wayfinderColor = IM_COL32(80, 220, 255, 230);
        constexpr float wayfinderRadius = 45.0f;
        // Direction arrow vector towards next checkpoint node
        const ImVec2 arrowHead(
            center.x + std::sin(m_hudWayfinderAngleRad) * wayfinderRadius,
            center.y - std::cos(m_hudWayfinderAngleRad) * wayfinderRadius);
        drawList->AddCircleFilled(arrowHead, 5.0f, wayfinderColor);
        drawList->AddLine(center, arrowHead, wayfinderColor, 2.0f);
        char distBuf[32];
        azsnprintf(distBuf, sizeof(distBuf), "GATE: %.0fm", static_cast<double>(m_hudDistanceToNextGateMeters));
        drawList->AddText(ImVec2(arrowHead.x - 20.0f, arrowHead.y + 8.0f), wayfinderColor, distBuf);

        // Weapon and vehicle status panel, bottom left.
        static const char* SlotNames[6] = {
            "FRONT L", "FRONT R", "CENTRAL", "LEFT", "RIGHT", "REAR"
        };
        static const char* ConditionNames[4] = {
            "INTACT", "DAMAGED", "CRITICAL", "DESTROYED"
        };

        ImGui::SetNextWindowPos(ImVec2(20.0f, displaySize.y - 170.0f));
        ImGui::SetNextWindowBgAlpha(0.35f);
        if (ImGui::Begin("CombatHud", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            for (int slot = 0; slot < 6; ++slot)
            {
                if (m_hudEquipped[slot] == 0)
                {
                    continue;
                }
                const ImVec4 ammoColor = m_hudAmmo[slot] > 0
                    ? ImVec4(0.6f, 1.0f, 0.6f, 1.0f)
                    : ImVec4(1.0f, 0.4f, 0.4f, 1.0f);
                ImGui::TextColored(ammoColor, "%s RIFLE  %u", SlotNames[slot],
                    m_hudAmmo[slot]);
            }

            const AZ::u32 conditionIndex = m_hudConditionState < 4
                ? m_hudConditionState : 3;
            ImGui::Text("CONDITION  %s (%.0f%%)", ConditionNames[conditionIndex],
                static_cast<double>(m_hudCondition * 100.0f));
            ImGui::Text("FUEL       %.0f%%",
                static_cast<double>(m_hudFuelRatio * 100.0f));
            ImGui::Text("SPEED      %.0f km/h",
                static_cast<double>(m_hudSpeedMps * 3.6f));
        }
        ImGui::End();

        // Race Standings and Lap Counter Banner, top right.
        ImGui::SetNextWindowPos(ImVec2(displaySize.x - 220.0f, 20.0f));
        ImGui::SetNextWindowBgAlpha(0.50f);
        if (ImGui::Begin("RaceStandingsHud", nullptr,
                ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings |
                ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav))
        {
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.2f, 1.0f), "RACE POSITION");
            ImGui::SetWindowFontScale(1.4f);
            ImGui::Text("POS: %u / 4", m_hudRaceRank > 0 ? m_hudRaceRank : 1);
            ImGui::SetWindowFontScale(1.0f);
            ImGui::Text("LAP: %u / 3", m_hudRaceLap > 0 ? m_hudRaceLap : 1);
        }
        ImGui::End();

        // Big Center-Screen Banner for Race Events (Countdown, Victory, Elimination)
        if (m_hudRaceState == 0) // Countdown
        {
            ImGui::SetNextWindowPos(ImVec2(center.x - 120.0f, center.y - 100.0f));
            ImGui::SetNextWindowBgAlpha(0.6f);
            if (ImGui::Begin("RaceCountdown", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::SetWindowFontScale(2.0f);
                if (m_hudCountdownTimer > 0.5f)
                {
                    ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  GET READY!");
                    ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.2f, 1.0f), "      %d", static_cast<int>(m_hudCountdownTimer) + 1);
                }
                else
                {
                    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "     GO!!!");
                }
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::End();
        }
        else if (m_hudRaceState == 2) // Victory / Race Finished
        {
            ImGui::SetNextWindowPos(ImVec2(center.x - 160.0f, center.y - 80.0f));
            ImGui::SetNextWindowBgAlpha(0.75f);
            if (ImGui::Begin("RaceVictory", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::SetWindowFontScale(1.8f);
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.3f, 1.0f), "FINISH LINE!");
                ImGui::TextColored(ImVec4(1.0f, 0.9f, 0.2f, 1.0f), "FINAL RANK: #%u", m_hudRaceRank);
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::End();
        }
        else if (m_hudRaceState == 3) // Elimination
        {
            ImGui::SetNextWindowPos(ImVec2(center.x - 160.0f, center.y - 80.0f));
            ImGui::SetNextWindowBgAlpha(0.85f);
            if (ImGui::Begin("RaceEliminated", nullptr,
                    ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoInputs |
                    ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings))
            {
                ImGui::SetWindowFontScale(1.8f);
                ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "VEHICLE DESTROYED!");
                ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "ELIMINATED FROM RACE");
                ImGui::SetWindowFontScale(1.0f);
            }
            ImGui::End();
        }
    }
#endif

} // namespace LDMChronoVehicle
