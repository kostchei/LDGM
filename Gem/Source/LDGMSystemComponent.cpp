
#include <AzCore/Serialization/SerializeContext.h>

#include "LDGMSystemComponent.h"

#include <LDGM/LDGMTypeIds.h>

namespace LDGM
{
    AZ_COMPONENT_IMPL(LDGMSystemComponent, "LDGMSystemComponent",
        LDGMSystemComponentTypeId);

    void LDGMSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LDGMSystemComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void LDGMSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        provided.push_back(AZ_CRC_CE("LDGMService"));
    }

    void LDGMSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        incompatible.push_back(AZ_CRC_CE("LDGMService"));
    }

    void LDGMSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
    }

    void LDGMSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
    }

    LDGMSystemComponent::LDGMSystemComponent()
    {
        if (LDGMInterface::Get() == nullptr)
        {
            LDGMInterface::Register(this);
        }
    }

    LDGMSystemComponent::~LDGMSystemComponent()
    {
        if (LDGMInterface::Get() == this)
        {
            LDGMInterface::Unregister(this);
        }
    }

    void LDGMSystemComponent::Init()
    {
    }

    void LDGMSystemComponent::Activate()
    {
        LDGMRequestBus::Handler::BusConnect();
    }

    void LDGMSystemComponent::Deactivate()
    {
        LDGMRequestBus::Handler::BusDisconnect();
    }
}
