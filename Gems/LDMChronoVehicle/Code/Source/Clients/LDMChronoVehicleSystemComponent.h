#pragma once

#include <memory>

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Component/EntityId.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/std/containers/unordered_map.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <LDMChronoVehicle/LDMChronoVehicleBus.h>
#include <Clients/PoseSnapshots.h>

#if defined(IMGUI_ENABLED)
#include <ImGuiBus.h>
#endif

namespace LDMChronoVehicle
{
    class LDMChronoVehicleSystemComponent
        : public AZ::Component
        , protected LDMChronoVehicleRequestBus::Handler
        , protected AZ::TickBus::Handler
#if defined(IMGUI_ENABLED)
        , protected ImGui::ImGuiUpdateListenerBus::Handler
#endif
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
        AZ::EntityId CreateClientVehiclePresentation(VehicleId vehicleId, const PoseSnapshotPacket& packet);
        AZ::EntityId CreateVehiclePartEntity(
            AZ::EntityId parentEntityId,
            const char* partName,
            const char* modelAssetPath,
            const AZ::Transform& localTransform,
            const AZ::Vector3& partSizeMeters);
        void EnsureClientTerrainPresentation();
        void EnsureClientCockpitCamera(AZ::EntityId playerVehicleEntityId);
        void DestroyClientPresentation();

#if defined(IMGUI_ENABLED)
        // ImGui::ImGuiUpdateListenerBus: fixed-forward reticle and weapon HUD.
        void OnImGuiUpdate() override;
#endif

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
        // Child part entities per vehicle rig (engine box, cabin panels,
        // cargo tray, wheels, weapon visuals). Destroyed with the rig.
        AZStd::unordered_map<VehicleId, AZStd::vector<AZ::EntityId>> m_clientVehiclePartEntities;
        AZ::EntityId m_clientTerrainEntityId;
        AZ::EntityId m_clientCameraEntityId;

        // Latest player HUD state mirrored from received snapshots.
        AZ::u32 m_hudAmmo[6] = { 0, 0, 0, 0, 0, 0 };
        AZ::u8 m_hudEquipped[6] = { 0, 0, 0, 0, 0, 0 };
        AZ::u32 m_hudConditionState = 0;
        float m_hudCondition = 1.0f;
        float m_hudFuelRatio = 1.0f;
        bool m_hudDisplayRequested = false;
        AZ::u32 m_cockpitScreenshotsTaken = 0;
        double m_nextSnapshotTraceTimeSeconds = 0.0;
        float m_maxClientProxyPositionErrorMeters = 0.0f;
        float m_maxClientProxyRotationErrorDegrees = 0.0f;

        // Combat Race HUD fields
        float m_hudWayfinderAngleRad = 0.0f;
        float m_hudDistanceToNextGateMeters = 0.0f;
        float m_hudSpeedMps = 0.0f;
        AZ::u32 m_hudRaceRank = 1;
        AZ::u32 m_hudRaceLap = 1;
        AZ::u32 m_hudRaceState = 0;
        float m_hudCountdownTimer = 3.0f;

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
