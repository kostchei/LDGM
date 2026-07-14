
#include <AzCore/Serialization/SerializeContext.h>
#include "LDMChronoVehicleEditorSystemComponent.h"

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

namespace LDMChronoVehicle
{
    AZ_COMPONENT_IMPL(LDMChronoVehicleEditorSystemComponent, "LDMChronoVehicleEditorSystemComponent",
        LDMChronoVehicleEditorSystemComponentTypeId, BaseSystemComponent);

    void LDMChronoVehicleEditorSystemComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<LDMChronoVehicleEditorSystemComponent, LDMChronoVehicleSystemComponent>()
                ->Version(0);
        }
    }

    LDMChronoVehicleEditorSystemComponent::LDMChronoVehicleEditorSystemComponent() = default;

    LDMChronoVehicleEditorSystemComponent::~LDMChronoVehicleEditorSystemComponent() = default;

    void LDMChronoVehicleEditorSystemComponent::GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided)
    {
        BaseSystemComponent::GetProvidedServices(provided);
        provided.push_back(AZ_CRC_CE("LDMChronoVehicleEditorService"));
    }

    void LDMChronoVehicleEditorSystemComponent::GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible)
    {
        BaseSystemComponent::GetIncompatibleServices(incompatible);
        incompatible.push_back(AZ_CRC_CE("LDMChronoVehicleEditorService"));
    }

    void LDMChronoVehicleEditorSystemComponent::GetRequiredServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        BaseSystemComponent::GetRequiredServices(required);
    }

    void LDMChronoVehicleEditorSystemComponent::GetDependentServices([[maybe_unused]] AZ::ComponentDescriptor::DependencyArrayType& dependent)
    {
        BaseSystemComponent::GetDependentServices(dependent);
    }

    void LDMChronoVehicleEditorSystemComponent::Activate()
    {
        LDMChronoVehicleSystemComponent::Activate();
        AzToolsFramework::EditorEvents::Bus::Handler::BusConnect();
    }

    void LDMChronoVehicleEditorSystemComponent::Deactivate()
    {
        AzToolsFramework::EditorEvents::Bus::Handler::BusDisconnect();
        LDMChronoVehicleSystemComponent::Deactivate();
    }

} // namespace LDMChronoVehicle
