
#pragma once

#include <AzCore/Component/Component.h>

#include <LDGM/LDGMBus.h>

namespace LDGM
{
    class LDGMSystemComponent
        : public AZ::Component
        , protected LDGMRequestBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(LDGMSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        LDGMSystemComponent();
        ~LDGMSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LDGMRequestBus interface implementation

        ////////////////////////////////////////////////////////////////////////

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////
    };
}
