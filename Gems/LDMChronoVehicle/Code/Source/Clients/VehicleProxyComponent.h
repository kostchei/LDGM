#pragma once

#include <AzCore/Component/Component.h>
#include <AzCore/Component/TickBus.h>
#include <AzCore/Math/Transform.h>

#include <LDMChronoVehicle/LDMChronoVehicleBus.h>

namespace LDMChronoVehicle
{
    // Presentation proxy for one authoritative vehicle (ADR 0002): each tick
    // after the simulation it pulls the authoritative chassis pose through
    // the Chrono-free Gem API, drives its entity transform, and records the
    // Chrono-versus-proxy error trace required by T0-PHYS-002. The entity
    // never runs a second authoritative rigid body.
    class VehicleProxyComponent
        : public AZ::Component
        , protected AZ::TickBus::Handler
    {
    public:
        AZ_COMPONENT_DECL(VehicleProxyComponent);

        static void Reflect(AZ::ReflectContext* context);
        static void GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required);

        explicit VehicleProxyComponent(VehicleId vehicleId = InvalidVehicleId);

        void SetVehicleId(VehicleId vehicleId);

    protected:
        void Init() override;
        void Activate() override;
        void Deactivate() override;

        void OnTick(float deltaTime, AZ::ScriptTimePoint time) override;
        int GetTickOrder() override;

    private:
        VehicleId m_vehicleId = InvalidVehicleId;
        double m_nextTraceTimeSeconds = 0.0;
        double m_elapsedSeconds = 0.0;
        float m_maxPositionErrorMeters = 0.0f;
        float m_maxRotationErrorDegrees = 0.0f;
    };
} // namespace LDMChronoVehicle
