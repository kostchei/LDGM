
#pragma once

#include <memory>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <LDMChronoVehicle/LDMChronoVehicleBus.h>

namespace LDMChronoVehicle
{
    class LDMChronoVehicleSystemComponent
        : public AZ::Component
        , protected LDMChronoVehicleRequestBus::Handler
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(LDMChronoVehicleSystemComponent);

        static void Reflect(AZ::ReflectContext* context);

        static void GetProvidedServices(AZ::ComponentDescriptor::DependencyArrayType& provided);
        static void GetIncompatibleServices(AZ::ComponentDescriptor::DependencyArrayType& incompatible);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);
        static void GetDependentServices(AZ::ComponentDescriptor::DependencyArrayType& dependent);

        LDMChronoVehicleSystemComponent();
        ~LDMChronoVehicleSystemComponent();

    protected:
        ////////////////////////////////////////////////////////////////////////
        // LDMChronoVehicleRequestBus interface implementation
        VehicleReservationResult ReserveActiveVehicle(VehicleId vehicleId) override;
        bool ReleaseActiveVehicle(VehicleId vehicleId) override;
        SimulationTelemetry GetSimulationTelemetry() const override;
        ////////////////////////////////////////////////////////////////////////

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        struct ChronoState;
        std::unique_ptr<ChronoState> m_chronoState;
    };

} // namespace LDMChronoVehicle
