
#include "LDMChronoVehicleSystemComponent.h"

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <Simulation/ActiveVehicleRegistry.h>
#include <Simulation/FixedStepClock.h>

#include <AzCore/Component/TickBus.h>
#include <AzCore/Console/IConsole.h>
#include <AzCore/Casting/numeric_cast.h>
#include <AzCore/Debug/Trace.h>
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
        ChronoState()
            : m_clock(FixedStepSeconds, MaxCatchUpSteps)
        {
            m_telemetry.m_isAuthoritative = true;
            m_telemetry.m_fixedStepSeconds = FixedStepSeconds;

            // O3DE and Chrono's default ISO world frame are both right-handed
            // Z-up, so poses cross the boundary without an axis permutation.
            AZ_Assert(chrono::vehicle::ChWorldFrame::IsISO(),
                "The Chrono world frame must remain the default ISO Z-up frame.");
            m_system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());

            m_probeBody = std::make_shared<chrono::ChBody>();
            m_probeBody->SetMass(1.0);
            m_probeBody->SetInertiaXX(chrono::ChVector3d(1.0, 1.0, 1.0));
            m_probeBody->SetPos(chrono::ChVector3d(0.0, 0.0, 2.0));
            m_system.AddBody(m_probeBody);
        }

        chrono::ChSystemNSC m_system;
        std::shared_ptr<chrono::ChBody> m_probeBody;
        FixedStepClock m_clock;
        ActiveVehicleRegistry m_activeVehicles;
        SimulationTelemetry m_telemetry;
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

        auto smokeState = std::make_unique<ChronoState>();
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
            m_chronoState = std::make_unique<ChronoState>();
            AZ::TickBus::Handler::BusConnect();
        }
    }

    void LDMChronoVehicleSystemComponent::Deactivate()
    {
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
    }

} // namespace LDMChronoVehicle
