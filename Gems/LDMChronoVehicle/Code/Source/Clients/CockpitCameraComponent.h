#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>

namespace LDMChronoVehicle
{
    // First-person cockpit camera for T0-CAM-003: the camera pose is derived
    // rigidly from the vehicle proxy entity every frame (proxy world
    // transform times a fixed cockpit offset). There is no other camera
    // path — no player-facing action can detach the camera or select an
    // external driving view, and the component logs the input-action
    // inventory proving that no such binding exists.
    //
    // Telemetry: per-second camera trace with NaN/invalid frame count and
    // maximum attachment error, matching the T0-CAM-003 assertions.
    class CockpitCameraComponent
        : public AZ::Component
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(CockpitCameraComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        explicit CockpitCameraComponent(AZ::EntityId targetVehicleEntityId = AZ::EntityId());
        void SetTargetVehicleEntityId(AZ::EntityId targetVehicleEntityId);

        // Driver eye point relative to the chassis reference frame.
        static AZ::Transform GetCockpitOffset();

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        void LogInputActionInventoryOnce();

        AZ::EntityId m_targetVehicleEntityId;
        AZ::Transform m_cameraWorldPose = AZ::Transform::CreateIdentity();
        double m_elapsedSeconds = 0.0;
        double m_nextTraceTimeSeconds = 0.0;
        AZ::u64 m_totalFrames = 0;
        AZ::u64 m_invalidFrames = 0;
        float m_maxAttachmentErrorMeters = 0.0f;
        bool m_inventoryLogged = false;
        bool m_activeCameraObserved = false;

        float m_headLookYawDegrees = 0.0f;
        bool m_fovModeWide = false;
        bool m_prevVToggled = false;
        bool m_cursorInitialized = false;
        int m_prevMouseX = 0;
    };
} // namespace LDMChronoVehicle
