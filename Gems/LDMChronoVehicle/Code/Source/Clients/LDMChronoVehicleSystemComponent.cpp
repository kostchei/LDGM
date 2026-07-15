
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
#include <Simulation/FixedStepClock.h>
#include <Simulation/TerrainFixtureConfig.h>
#include <Simulation/VehicleFixture.h>

#include <AzCore/Component/Entity.h>
#include <AzCore/Component/TransformBus.h>
#include <AzFramework/Components/TransformComponent.h>
#include <AzFramework/Components/CameraBus.h>
#include <AzFramework/Components/NonUniformScaleComponent.h>
#include <AzFramework/Entity/GameEntityContextBus.h>

#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentBus.h>
#include <AtomLyIntegration/CommonFeatures/Mesh/MeshComponentConstants.h>

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
        constexpr const char* VehicleModelAssetPath =
            "materialeditor/viewportmodels/cube.fbx.azmodel";
        constexpr const char* GroundModelAssetPath =
            "objects/groudplane/groundplane_512x512m.fbx.azmodel";

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
        }
    }

    void LDMChronoVehicleSystemComponent::Deactivate()
    {
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
                        inputs.m_fireLeft = inputPacket.m_fireLeft != 0;
                        inputs.m_fireRight = inputPacket.m_fireRight != 0;
                        vehicle.m_fixture->SetLiveInputs(inputs);
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
                    m_chronoState->m_snapshotPublisher->Publish(
                        simulationTime,
                        vehicle.m_vehicleId,
                        vehicle.m_fixture->GetChassisPose(),
                        telemetry.m_terrainChecksum,
                        vehicle.m_fixture->GetLeftRifleAmmo(),
                        vehicle.m_fixture->GetRightRifleAmmo(),
                        vehicle.m_fixture->GetConditionState(),
                        vehicle.m_fixture->GetCondition());
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
        bool targetFireLeft = false;
        bool targetFireRight = false;

#if defined(AZ_PLATFORM_WINDOWS)
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
            targetFireLeft = true;
        }
        if (GetAsyncKeyState('E') & 0x8000 || GetAsyncKeyState(VK_RBUTTON) & 0x8000)
        {
            targetFireRight = true;
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
                    targetFireLeft = true;
                }
                if (state.Gamepad.wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
                {
                    targetFireRight = true;
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
                targetFireLeft, targetFireRight);
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
                entityId = CreateClientVehiclePresentation(packet.m_vehicleId);
            }
            if (!entityId.IsValid())
            {
                continue;
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
            AZ::Data::AssetId modelAssetId;
            AZ::Render::MeshComponentRequestBus::EventResult(
                modelAssetId, entityId,
                &AZ::Render::MeshComponentRequestBus::Events::GetModelAssetId);
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
                    "T1 warning trace: t=%.3f state=%u condition=%.3f left_ammo=%u right_ammo=%u\n",
                    packet.m_simulationTimeSeconds,
                    packet.m_conditionState,
                    static_cast<double>(packet.m_condition),
                    packet.m_leftAmmo,
                    packet.m_rightAmmo);

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
            }
        }
    }

    AZ::EntityId LDMChronoVehicleSystemComponent::CreateClientVehiclePresentation(
        VehicleId vehicleId)
    {
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
        entity->CreateComponent<AzFramework::NonUniformScaleComponent>();
        AZ::Component* meshComponent =
            entity->CreateComponent(AZ::Render::MeshComponentTypeId);
        if (meshComponent == nullptr)
        {
            AZ_Error("LDMChronoVehicle", false,
                "The Atom runtime MeshComponent descriptor is unavailable.");
            AzFramework::GameEntityContextRequestBus::Broadcast(
                &AzFramework::GameEntityContextRequestBus::Events::DestroyGameEntity,
                entity->GetId());
            return AZ::EntityId();
        }

        entity->Activate();
        AZ::NonUniformScaleRequestBus::Event(
            entity->GetId(), &AZ::NonUniformScaleRequests::SetScale,
            AZ::Vector3(4.0f, 2.0f, 1.0f));
        AZ::Render::MeshComponentRequestBus::Event(
            entity->GetId(),
            &AZ::Render::MeshComponentRequestBus::Events::SetModelAssetPath,
            AZStd::string(VehicleModelAssetPath));
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

        entity->Activate();
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

        entity->Activate();
        Camera::CameraRequestBus::Event(
            entity->GetId(), &Camera::CameraRequestBus::Events::MakeActiveView);
        m_clientCameraEntityId = entity->GetId();
        AZ_Printf("LDMChronoVehicle",
            "T0 cockpit camera created: camera_component=true target_vehicle=%llu.\n",
            static_cast<unsigned long long>(ChronoState::PlayerVehicleId));
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

} // namespace LDMChronoVehicle
