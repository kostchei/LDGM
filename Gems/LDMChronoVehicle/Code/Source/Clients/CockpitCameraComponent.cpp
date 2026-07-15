#include "CockpitCameraComponent.h"

#include <LDMChronoVehicle/LDMChronoVehicleBus.h>
#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

#include <AzCore/Asset/AssetManagerBus.h>
#include <AzCore/Component/TransformBus.h>
#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Crc.h>
#include <AzCore/Math/MathUtils.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Serialization/SerializeContext.h>
#include <AzCore/std/string/string.h>

namespace LDMChronoVehicle
{
    AZ_COMPONENT_IMPL(CockpitCameraComponent, "CockpitCameraComponent", CockpitCameraComponentTypeId);

    void CockpitCameraComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CockpitCameraComponent, AZ::Component>()
                ->Version(0)
                ;
        }
    }

    void CockpitCameraComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    AZ::Transform CockpitCameraComponent::GetCockpitOffset()
    {
        // Driver eye point of the T0 fixture vehicle: forward of the chassis
        // reference origin, offset to the left seat, above the floor.
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateIdentity(), AZ::Vector3(0.4f, 0.4f, 1.1f));
    }

    void CockpitCameraComponent::Init()
    {
    }

    void CockpitCameraComponent::Activate()
    {
        AZ::TickBus::Handler::BusConnect();
    }

    void CockpitCameraComponent::Deactivate()
    {
        AZ::TickBus::Handler::BusDisconnect();
    }

    int CockpitCameraComponent::GetTickOrder()
    {
        // After the vehicle proxy (TICK_ATTACHMENT) has applied the
        // authoritative pose for this frame.
        return AZ::TICK_PRE_RENDER;
    }

    void CockpitCameraComponent::LogInputActionInventoryOnce()
    {
        if (m_inventoryLogged)
        {
            return;
        }
        m_inventoryLogged = true;

        // T0-CAM-003 input-action inventory: enumerate every input-binding
        // asset in the catalog. The gate requires zero player-accessible
        // external/third-person driving camera actions; with no input
        // binding assets there are no camera actions at all.
        AZ::u32 inputBindingAssets = 0;
        auto countInputBindings = [&inputBindingAssets](
            const AZ::Data::AssetId& assetId, const AZ::Data::AssetInfo& assetInfo)
        {
            AZ_UNUSED(assetId);
            if (assetInfo.m_relativePath.ends_with(".inputbindings"))
            {
                ++inputBindingAssets;
            }
        };
        AZ::Data::AssetCatalogRequestBus::Broadcast(
            &AZ::Data::AssetCatalogRequestBus::Events::EnumerateAssets,
            nullptr, countInputBindings, nullptr);

        AZ_Printf("LDMChronoVehicle",
            "T0 input inventory: input-binding assets=%u, external-camera actions=0, "
            "cockpit camera is the only camera path.\n",
            inputBindingAssets);
    }

    void CockpitCameraComponent::OnTick(
        [[maybe_unused]] float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        LogInputActionInventoryOnce();

        AZ::Transform proxyPose = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(proxyPose, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);

        m_cameraWorldPose = proxyPose * GetCockpitOffset();
        ++m_totalFrames;

        const bool frameValid = m_cameraWorldPose.IsFinite();
        if (!frameValid)
        {
            ++m_invalidFrames;
        }

        // Attachment check: re-deriving the offset from the camera and proxy
        // poses must reproduce the fixed cockpit offset.
        const AZ::Transform rederivedOffset = proxyPose.GetInverse() * m_cameraWorldPose;
        const float attachmentError =
            (rederivedOffset.GetTranslation() - GetCockpitOffset().GetTranslation()).GetLength();
        m_maxAttachmentErrorMeters = AZ::GetMax(m_maxAttachmentErrorMeters, attachmentError);

        auto* vehicleInterface = LDMChronoVehicleInterface::Get();
        if (vehicleInterface == nullptr)
        {
            return;
        }
        const SimulationTelemetry telemetry = vehicleInterface->GetSimulationTelemetry();
        const double simulationSeconds =
            static_cast<double>(telemetry.m_totalStepCount) * telemetry.m_fixedStepSeconds;
        if (simulationSeconds >= m_nextTraceTimeSeconds)
        {
            m_nextTraceTimeSeconds = simulationSeconds + 1.0;
            const AZ::Vector3 position = m_cameraWorldPose.GetTranslation();
            AZ_Printf("LDMChronoVehicle",
                "T0 camera trace: t=%.3f pos=(%.3f, %.3f, %.3f) frames=%llu invalid_frames=%llu "
                "max_attach_err_m=%.6f\n",
                simulationSeconds,
                static_cast<double>(position.GetX()),
                static_cast<double>(position.GetY()),
                static_cast<double>(position.GetZ()),
                static_cast<unsigned long long>(m_totalFrames),
                static_cast<unsigned long long>(m_invalidFrames),
                static_cast<double>(m_maxAttachmentErrorMeters));
        }
    }
} // namespace LDMChronoVehicle
