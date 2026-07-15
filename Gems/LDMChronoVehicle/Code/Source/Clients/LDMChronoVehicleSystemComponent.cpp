
#include "LDMChronoVehicleSystemComponent.h"

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

#include <AzCore/Debug/Trace.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <chrono/physics/ChBody.h>
#include <chrono/physics/ChSystemNSC.h>
#include <chrono_vehicle/ChWorldFrame.h>

#include <cmath>
#include <memory>

namespace LDMChronoVehicle
{
    struct LDMChronoVehicleSystemComponent::ChronoState
    {
        ChronoState()
        {
            chrono::vehicle::ChWorldFrame::SetYUP();
            m_system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());

            m_probeBody = std::make_shared<chrono::ChBody>();
            m_probeBody->SetMass(1.0);
            m_probeBody->SetInertiaXX(chrono::ChVector3d(1.0, 1.0, 1.0));
            m_probeBody->SetPos(chrono::ChVector3d(0.0, 2.0, 0.0));
            m_system.AddBody(m_probeBody);
        }

        chrono::ChSystemNSC m_system;
        std::shared_ptr<chrono::ChBody> m_probeBody;
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

        m_chronoState = std::make_unique<ChronoState>();
        constexpr double smokeStep = 0.005;
        m_chronoState->m_system.DoStepDynamics(smokeStep);

        const double height = chrono::vehicle::ChWorldFrame::Height(m_chronoState->m_probeBody->GetPos());
        const bool smokePassed = std::isfinite(height) &&
            std::abs(m_chronoState->m_system.GetChTime() - smokeStep) < 1e-12 && height < 2.0;
        AZ_Error("LDMChronoVehicle", smokePassed,
            "Chrono Core/Vehicle lifecycle smoke failed (time=%f, height=%f).",
            m_chronoState->m_system.GetChTime(), height);
        if (smokePassed)
        {
            const double simulationTime = m_chronoState->m_system.GetChTime();
            AZ::TickBus::QueueFunction([simulationTime, height]()
            {
                AZ_Printf("LDMChronoVehicle", "Chrono Core/Vehicle lifecycle smoke passed (time=%f, height=%f).\n",
                    simulationTime, height);
            });
        }
    }

    void LDMChronoVehicleSystemComponent::Deactivate()
    {
        m_chronoState.reset();
        LDMChronoVehicleRequestBus::Handler::BusDisconnect();
    }

} // namespace LDMChronoVehicle
