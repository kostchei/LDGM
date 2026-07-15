#pragma once

#include <memory>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <LDMChronoVehicle/LDMChronoVehicleBus.h>
#include <Clients/PoseSnapshots.h>

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
        bool GetActiveVehiclePose(VehicleId vehicleId, AZ::Transform& outPose) const override;
        ////////////////////////////////////////////////////////////////////////

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;
        void OnClientTick();
        AZ::EntityId CreateClientVehiclePresentation(VehicleId vehicleId);
        void EnsureClientTerrainPresentation();
        void EnsureClientCockpitCamera(AZ::EntityId playerVehicleEntityId);
        void DestroyClientPresentation();

        ////////////////////////////////////////////////////////////////////////
        // AZ::Component interface implementation
        void Init() override;
        void Activate() override;
        void Deactivate() override;
        ////////////////////////////////////////////////////////////////////////

    private:
        struct ChronoState;
        std::unique_ptr<ChronoState> m_chronoState;

        // Client role: consumes authoritative pose snapshots.
        AZStd::unique_ptr<PoseSnapshotReceiver> m_snapshotReceiver;
        AZStd::unique_ptr<VehicleInputPublisher> m_inputPublisher;
        AZStd::unique_ptr<VehicleInputReceiver> m_inputReceiver;
        AZStd::unordered_map<VehicleId, AZ::EntityId> m_clientVehicleEntities;
        AZ::EntityId m_clientTerrainEntityId;
        AZ::EntityId m_clientCameraEntityId;
        double m_nextSnapshotTraceTimeSeconds = 0.0;
        float m_maxClientProxyPositionErrorMeters = 0.0f;
        float m_maxClientProxyRotationErrorDegrees = 0.0f;

        // Tracks client's local input state:
        double m_clientSteering = 0.0;
        double m_clientThrottle = 0.0;
        double m_clientBrake = 0.0;
        bool m_clientHandbrake = false;
        int m_clientDriveMode = 1;
        bool m_clientEngineStarted = true;

        bool m_prevGearUpPressed = false;
        bool m_prevGearDownPressed = false;
        bool m_prevEngineStartPressed = false;
    };

} // namespace LDMChronoVehicle
