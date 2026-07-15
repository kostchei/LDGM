
#include "LDMChronoVehicleModuleInterface.h"
#include <AzCore/Memory/Memory.h>

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

#include <Clients/CockpitCameraComponent.h>
#include <Clients/LDMChronoVehicleSystemComponent.h>
#include <Clients/VehicleProxyComponent.h>

namespace LDMChronoVehicle
{
    AZ_TYPE_INFO_WITH_NAME_IMPL(LDMChronoVehicleModuleInterface,
        "LDMChronoVehicleModuleInterface", LDMChronoVehicleModuleInterfaceTypeId);
    AZ_RTTI_NO_TYPE_INFO_IMPL(LDMChronoVehicleModuleInterface, AZ::Module);
    AZ_CLASS_ALLOCATOR_IMPL(LDMChronoVehicleModuleInterface, AZ::SystemAllocator);

    LDMChronoVehicleModuleInterface::LDMChronoVehicleModuleInterface()
    {
        // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
        // Add ALL components descriptors associated with this gem to m_descriptors.
        // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
        // This happens through the [MyComponent]::Reflect() function.
        m_descriptors.insert(m_descriptors.end(), {
            LDMChronoVehicleSystemComponent::CreateDescriptor(),
            VehicleProxyComponent::CreateDescriptor(),
            CockpitCameraComponent::CreateDescriptor(),
            });
    }

    AZ::ComponentTypeList LDMChronoVehicleModuleInterface::GetRequiredSystemComponents() const
    {
        return AZ::ComponentTypeList{
            azrtti_typeid<LDMChronoVehicleSystemComponent>(),
        };
    }
} // namespace LDMChronoVehicle
