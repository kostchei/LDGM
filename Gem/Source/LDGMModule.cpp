
#include <AzCore/Memory/SystemAllocator.h>
#include <AzCore/Module/Module.h>

#include "LDGMSystemComponent.h"

#include <LDGM/LDGMTypeIds.h>

namespace LDGM
{
    class LDGMModule
        : public AZ::Module
    {
    public:
        AZ_RTTI(LDGMModule, LDGMModuleTypeId, AZ::Module);
        AZ_CLASS_ALLOCATOR(LDGMModule, AZ::SystemAllocator);

        LDGMModule()
            : AZ::Module()
        {
            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            m_descriptors.insert(m_descriptors.end(), {
                LDGMSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList{
                azrtti_typeid<LDGMSystemComponent>(),
            };
        }
    };
}// namespace LDGM

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LDGM::LDGMModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LDGM, LDGM::LDGMModule)
#endif
