#include "VehicleProxyComponent.h"

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>

#include <cmath>

namespace LDMChronoVehicle
{
    AZ_COMPONENT_IMPL(VehicleProxyComponent, "VehicleProxyComponent", VehicleProxyComponentTypeId);

    void VehicleProxyComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<VehicleProxyComponent, AZ::Component>()
                ->Version(0)
                ->Field("VehicleId", &VehicleProxyComponent::m_vehicleId)
                ;
        }
    }

    void VehicleProxyComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    VehicleProxyComponent::VehicleProxyComponent(VehicleId vehicleId)
        : m_vehicleId(vehicleId)
    {
    }

    void VehicleProxyComponent::SetVehicleId(VehicleId vehicleId)
    {
        m_vehicleId = vehicleId;
    }

    void VehicleProxyComponent::Init()
    {
    }

    void VehicleProxyComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void VehicleProxyComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    int VehicleProxyComponent::GetTickOrder()
    {
        // The authoritative simulation ticks at TICK_PHYSICS; the proxy
        // consumes its result within the same frame.
        return AZ::TICK_ATTACHMENT;
    }

    void VehicleProxyComponent::OnTick(
        [[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        auto* vehicleInterface = LDMChronoVehicleInterface::Get();
        if (vehicleInterface == nullptr || m_vehicleId == InvalidVehicleId)
        {
            return;
        }

        AZ::Transform authoritativePose = AZ::Transform::CreateIdentity();
        if (!vehicleInterface->GetActiveVehiclePose(m_vehicleId, authoritativePose))
        {
            return;
        }

        AZ::TransformBus::Event(GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, authoritativePose);

        // T0-PHYS-002 evidence: measure what the entity actually carries
        // against the authoritative pose it was fed.
        AZ::Transform appliedPose = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(appliedPose, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        const float positionError =
            (appliedPose.GetTranslation() - authoritativePose.GetTranslation()).GetLength();
        const AZ::Quaternion rotationDelta =
            appliedPose.GetRotation().GetConjugate() * authoritativePose.GetRotation();
        const float rotationErrorDegrees = AZ::RadToDeg(
            2.0f * std::acos(AZ::GetClamp(std::abs(rotationDelta.GetW()), 0.0f, 1.0f)));

        m_maxPositionErrorMeters = AZ::GetMax(m_maxPositionErrorMeters, positionError);
        m_maxRotationErrorDegrees = AZ::GetMax(m_maxRotationErrorDegrees, rotationErrorDegrees);

        // Trace in authoritative simulation time so proxy lines align with
        // the simulation's own transform trace.
        const SimulationTelemetry telemetry = vehicleInterface->GetSimulationTelemetry();
        m_elapsedSeconds =
            static_cast<double>(telemetry.m_totalStepCount) * telemetry.m_fixedStepSeconds;
        if (m_elapsedSeconds >= m_nextTraceTimeSeconds)
        {
            m_nextTraceTimeSeconds = m_elapsedSeconds + 1.0;
            const AZ::Vector3 position = appliedPose.GetTranslation();
            AZ_Printf("LDMChronoVehicle",
                "T0 proxy trace: t=%.3f pos=(%.3f, %.3f, %.3f) err_pos_m=%.6f err_rot_deg=%.6f "
                "max_err_pos_m=%.6f max_err_rot_deg=%.6f\n",
                m_elapsedSeconds,
                static_cast<double>(position.GetX()),
                static_cast<double>(position.GetY()),
                static_cast<double>(position.GetZ()),
                static_cast<double>(positionError),
                static_cast<double>(rotationErrorDegrees),
                static_cast<double>(m_maxPositionErrorMeters),
                static_cast<double>(m_maxRotationErrorDegrees));
        }
    }
} // namespace LDMChronoVehicle
