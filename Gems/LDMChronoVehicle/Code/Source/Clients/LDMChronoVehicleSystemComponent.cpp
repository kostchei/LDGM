
#include "LDMChronoVehicleSystemComponent.h"

#include <Clients/CockpitCameraComponent.h>
#include <Clients/VehicleProxyComponent.h>
#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <Simulation/ActiveVehicleRegistry.h>
#include <Simulation/FixedStepClock.h>
#include <Simulation/TerrainFixtureConfig.h>
#include <Simulation/VehicleFixture.h>

#include <AzCore/Component/Entity.h>
#include <AzFramework/Components/TransformComponent.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/Settings/SettingsRegistry.h>
#include <AzCore/Settings/SettingsRegistryMergeUtils.h>
#include <AzCore/std/string/string.h>

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

        // The T0 fixture vehicle occupies one slot of the active registry;
        // capacity is reserved before the Chrono vehicle exists (ADR 0002).
        static constexpr VehicleId FixtureVehicleId = 1;

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

            const VehicleReservationResult reservation = m_activeVehicles.Reserve(FixtureVehicleId);
            AZ_Assert(reservation == VehicleReservationResult::Reserved,
                "Reserving the T0 fixture vehicle failed (result=%d).",
                static_cast<int>(reservation));
            m_vehicleFixture = std::make_unique<VehicleFixture>(m_system, FixedStepSeconds);

            m_telemetry.m_activeVehicleCount =
                aznumeric_cast<AZ::u32>(m_activeVehicles.GetActiveCount());
            m_telemetry.m_vehicleFixtureActive = true;
            m_telemetry.m_terrainChecksum = CalculateTerrainFixtureChecksum();
        }

        chrono::ChSystemNSC m_system;
        std::shared_ptr<chrono::ChBody> m_probeBody;
        std::unique_ptr<VehicleFixture> m_vehicleFixture;
        FixedStepClock m_clock;
        ActiveVehicleRegistry m_activeVehicles;
        SimulationTelemetry m_telemetry;
        double m_nextTraceTimeSeconds = 0.0;
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
            AZ_Printf("LDMChronoVehicle",
                "T0 vehicle fixture created (terrain checksum=0x%08X, vehicles=%u/%u).\n",
                m_chronoState->m_telemetry.m_terrainChecksum,
                m_chronoState->m_telemetry.m_activeVehicleCount,
                MaxActiveVehicles);
            AZ::TickBus::Handler::BusConnect();

            // Presentation proxy for the fixture vehicle. On the T0 local
            // integration host it lives in the authoritative process; the
            // networked client gains the same entity via snapshots later.
            m_proxyEntity = AZStd::make_unique<AZ::Entity>("T0VehicleProxy");
            m_proxyEntity->CreateComponent<AzFramework::TransformComponent>();
            m_proxyEntity->CreateComponent<VehicleProxyComponent>(ChronoState::FixtureVehicleId);
            m_proxyEntity->CreateComponent<CockpitCameraComponent>();
            m_proxyEntity->Init();
            m_proxyEntity->Activate();
        }
    }

    void LDMChronoVehicleSystemComponent::Deactivate()
    {
        if (m_proxyEntity)
        {
            m_proxyEntity->Deactivate();
            m_proxyEntity.reset();
        }
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
        if (!m_chronoState || !m_chronoState->m_vehicleFixture ||
            !m_chronoState->m_activeVehicles.Contains(vehicleId) ||
            vehicleId != ChronoState::FixtureVehicleId)
        {
            return false;
        }

        outPose = m_chronoState->m_vehicleFixture->GetChassisPose();
        return true;
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
            return;
        }

        const FixedStepPlan plan = m_chronoState->m_clock.Advance(deltaTime);
        const auto stepStartedAt = std::chrono::steady_clock::now();
        for (AZ::u32 step = 0; step < plan.m_stepCount; ++step)
        {
            if (m_chronoState->m_vehicleFixture)
            {
                m_chronoState->m_vehicleFixture->SynchronizeAndAdvance(
                    m_chronoState->m_system.GetChTime());
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

        if (m_chronoState->m_vehicleFixture && plan.m_stepCount > 0)
        {
            const AZ::Transform pose = m_chronoState->m_vehicleFixture->GetChassisPose();
            const AZ::Vector3 position = pose.GetTranslation();
            telemetry.m_vehiclePositionX = static_cast<double>(position.GetX());
            telemetry.m_vehiclePositionY = static_cast<double>(position.GetY());
            telemetry.m_vehiclePositionZ = static_cast<double>(position.GetZ());
            telemetry.m_vehicleSpeedMetersPerSecond =
                m_chronoState->m_vehicleFixture->GetForwardSpeedMetersPerSecond();

            // Transform trace for T0-PHYS-002 evidence: one line per
            // simulated second in the server log.
            const double simulationTime = m_chronoState->m_system.GetChTime();
            if (simulationTime >= m_chronoState->m_nextTraceTimeSeconds)
            {
                m_chronoState->m_nextTraceTimeSeconds = simulationTime + 1.0;
                AZ_Printf("LDMChronoVehicle",
                    "T0 transform trace: t=%.3f pos=(%.3f, %.3f, %.3f) speed=%.2f terrain=0x%08X\n",
                    simulationTime,
                    telemetry.m_vehiclePositionX,
                    telemetry.m_vehiclePositionY,
                    telemetry.m_vehiclePositionZ,
                    telemetry.m_vehicleSpeedMetersPerSecond,
                    telemetry.m_terrainChecksum);
            }
        }
    }

} // namespace LDMChronoVehicle
