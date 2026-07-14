
#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <LDMChronoVehicleModuleInterface.h>
#include "LDMChronoVehicleEditorSystemComponent.h"

namespace LDMChronoVehicle
{
    class LDMChronoVehicleEditorModule
        : public LDMChronoVehicleModuleInterface
    {
    public:
        AZ_RTTI(LDMChronoVehicleEditorModule, LDMChronoVehicleEditorModuleTypeId, LDMChronoVehicleModuleInterface);
        AZ_CLASS_ALLOCATOR(LDMChronoVehicleEditorModule, AZ::SystemAllocator);

        LDMChronoVehicleEditorModule()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                LDMChronoVehicleEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<LDMChronoVehicleEditorSystemComponent>(),
            };
        }
    };
}// namespace LDMChronoVehicle

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), LDMChronoVehicle::LDMChronoVehicleEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LDMChronoVehicle_Editor, LDMChronoVehicle::LDMChronoVehicleEditorModule)
#endif
