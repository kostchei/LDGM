
#pragma once

#include <AzToolsFramework/API/ToolsApplicationAPI.h>

#include <Clients/LDMChronoVehicleSystemComponent.h>

namespace LDMChronoVehicle
{
    /// System component for LDMChronoVehicle editor
    class LDMChronoVehicleEditorSystemComponent
        : public LDMChronoVehicleSystemComponent
        , protected AzToolsFramework::EditorEvents::Bus::Handler
    {
        using BaseSystemComponent = LDMChronoVehicleSystemComponent;
    public:
        AZ_COMPONENT_DECL(LDMChronoVehicleEditorSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        LDMChronoVehicleEditorSystemComponent();
        ~LDMChronoVehicleEditorSystemComponent();

    private:
        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        // AZ::Component
        void Activate() override;
        void Deactivate() override;
    };
} // namespace LDMChronoVehicle
