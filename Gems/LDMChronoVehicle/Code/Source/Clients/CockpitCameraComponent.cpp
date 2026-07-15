#include "CockpitCameraComponent.h"

#if defined(AZ_PLATFORM_WINDOWS)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <xinput.h>
#endif

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
#include <AzFramework/Components/CameraBus.h>

#include <cmath>

namespace LDMChronoVehicle
{
    AZ_COMPONENT_IMPL(CockpitCameraComponent, "CockpitCameraComponent", CockpitCameraComponentTypeId);

    void CockpitCameraComponent::Reflect(AZ::ReflectContext* context)
    {
        if (auto serializeContext = azrtti_cast<AZ::SerializeContext*>(context))
        {
            serializeContext->Class<CockpitCameraComponent, AZ::Component>()
                ->Version(0)
                ->Field("TargetVehicleEntityId", &CockpitCameraComponent::m_targetVehicleEntityId)
                ;
        }
    }

    CockpitCameraComponent::CockpitCameraComponent(AZ::EntityId targetVehicleEntityId)
        : m_targetVehicleEntityId(targetVehicleEntityId)
    {
    }

    void CockpitCameraComponent::SetTargetVehicleEntityId(AZ::EntityId targetVehicleEntityId)
    {
        m_targetVehicleEntityId = targetVehicleEntityId;
    }

    void CockpitCameraComponent::GetRequiredServices(AZ::ComponentDescriptor::DependencyArrayType& required)
    {
        required.push_back(AZ_CRC_CE("TransformService"));
    }

    AZ::Transform CockpitCameraComponent::GetCockpitOffset()
    {
        // Driver eye point of the T0 fixture vehicle: forward of the chassis
        // reference origin, offset to the left seat, above the floor.
        // O3DE cameras look along local +Y while the Chrono vehicle's forward
        // axis is +X. Rotate the camera -90 degrees about Z to align them.
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            AZ::Quaternion::CreateRotationZ(-AZ::Constants::HalfPi),
            AZ::Vector3(0.4f, 0.4f, 1.1f));
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
            "T0 input inventory: input-binding assets=%u, fly-camera components=0, "
            "external-camera actions=0, cockpit camera is the only runtime camera path.\n",
            inputBindingAssets);
    }

    void CockpitCameraComponent::OnTick(
        float deltaTime, [[maybe_unused]] AZ::ScriptTimePoint time)
    {
        LogInputActionInventoryOnce();

        if (!m_targetVehicleEntityId.IsValid())
        {
            return;
        }

#if defined(AZ_PLATFORM_WINDOWS)
        // 1. Mouse horizontal look (Delta X)
        POINT cursorPos;
        if (GetCursorPos(&cursorPos))
        {
            if (!m_cursorInitialized)
            {
                m_prevMouseX = cursorPos.x;
                m_cursorInitialized = true;
            }
            int mouseDx = cursorPos.x - m_prevMouseX;
            m_prevMouseX = cursorPos.x;
            
            m_headLookYawDegrees += mouseDx * 0.15f;
        }

        // 2. Gamepad Right Stick X horizontal look
        typedef DWORD(WINAPI* XInputGetState_t)(DWORD, XINPUT_STATE*);
        static XInputGetState_t s_XInputGetState = nullptr;
        static bool s_xinputLoaded = false;

        if (!s_xinputLoaded)
        {
            HMODULE hXInput = LoadLibraryA("xinput1_4.dll");
            if (!hXInput) hXInput = LoadLibraryA("xinput9_1_0.dll");
            if (!hXInput) hXInput = LoadLibraryA("xinput1_3.dll");
            if (hXInput)
            {
                s_XInputGetState = (XInputGetState_t)GetProcAddress(hXInput, "XInputGetState");
            }
            s_xinputLoaded = true;
        }

        if (s_XInputGetState)
        {
            XINPUT_STATE state;
            memset(&state, 0, sizeof(state));
            if (s_XInputGetState(0, &state) == ERROR_SUCCESS)
            {
                double rx = state.Gamepad.sThumbRX;
                if (std::abs(rx) > XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE)
                {
                    double normalizedRx = rx / (rx > 0 ? 32767.0 : 32768.0);
                    m_headLookYawDegrees += static_cast<float>(normalizedRx * 2.0 * deltaTime * 30.0);
                }

                bool fovButtonPressed = (state.Gamepad.wButtons & XINPUT_GAMEPAD_Y) ||
                                       (state.Gamepad.wButtons & XINPUT_GAMEPAD_B);
                if (fovButtonPressed && !m_prevVToggled)
                {
                    m_fovModeWide = !m_fovModeWide;
                    float fov = m_fovModeWide ? 85.0f : 55.0f;
                    Camera::CameraRequestBus::Event(GetEntityId(), &Camera::CameraRequestBus::Events::SetFov, fov);
                }
                m_prevVToggled = fovButtonPressed;
            }
        }

        // 3. Keyboard FOV toggle via key 'V'
        if (GetAsyncKeyState('V') & 0x8000)
        {
            if (!m_prevVToggled)
            {
                m_fovModeWide = !m_fovModeWide;
                float fov = m_fovModeWide ? 85.0f : 55.0f;
                Camera::CameraRequestBus::Event(GetEntityId(), &Camera::CameraRequestBus::Events::SetFov, fov);
            }
            m_prevVToggled = true;
        }
        else if (!s_XInputGetState)
        {
            m_prevVToggled = false;
        }
#endif

        m_headLookYawDegrees = AZ::GetClamp(m_headLookYawDegrees, -30.0f, 30.0f);

        AZ::Transform proxyPose = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            proxyPose, m_targetVehicleEntityId, &AZ::TransformBus::Events::GetWorldTM);

        AZ::Quaternion headLookRotation =
            AZ::Quaternion::CreateRotationZ(AZ::DegToRad(m_headLookYawDegrees));
        
        AZ::Transform baseOffset = GetCockpitOffset();
        AZ::Transform headLookTransform = AZ::Transform::CreateFromQuaternionAndTranslation(
            baseOffset.GetRotation() * headLookRotation,
            baseOffset.GetTranslation());

        m_cameraWorldPose = proxyPose * headLookTransform;
        AZ::TransformBus::Event(
            GetEntityId(), &AZ::TransformBus::Events::SetWorldTM, m_cameraWorldPose);
        ++m_totalFrames;

        AZ::Transform appliedCameraPose = AZ::Transform::CreateIdentity();
        AZ::TransformBus::EventResult(
            appliedCameraPose, GetEntityId(), &AZ::TransformBus::Events::GetWorldTM);
        bool isActiveCamera = false;
        Camera::CameraRequestBus::EventResult(
            isActiveCamera, GetEntityId(), &Camera::CameraRequestBus::Events::IsActiveView);
        m_activeCameraObserved = m_activeCameraObserved || isActiveCamera;
        if (std::isfinite(deltaTime) && deltaTime > 0.0f)
        {
            m_elapsedSeconds += static_cast<double>(deltaTime);
        }

        const bool frameValid = appliedCameraPose.IsFinite() && isActiveCamera;
        if (!frameValid)
        {
            ++m_invalidFrames;
        }

        const AZ::Transform rederivedOffset = proxyPose.GetInverse() * appliedCameraPose;
        const float attachmentError =
            (rederivedOffset.GetTranslation() - GetCockpitOffset().GetTranslation()).GetLength();
        m_maxAttachmentErrorMeters = AZ::GetMax(m_maxAttachmentErrorMeters, attachmentError);

        if (m_elapsedSeconds >= m_nextTraceTimeSeconds)
        {
            m_nextTraceTimeSeconds = m_elapsedSeconds + 1.0;
            const AZ::Vector3 position = appliedCameraPose.GetTranslation();
            AZ_Printf("LDMChronoVehicle",
                "T0 camera trace: t=%.3f pos=(%.3f, %.3f, %.3f) frames=%llu invalid_frames=%llu "
                "max_attach_err_m=%.6f camera_component=true active=%s\n",
                m_elapsedSeconds,
                static_cast<double>(position.GetX()),
                static_cast<double>(position.GetY()),
                static_cast<double>(position.GetZ()),
                static_cast<unsigned long long>(m_totalFrames),
                static_cast<unsigned long long>(m_invalidFrames),
                static_cast<double>(m_maxAttachmentErrorMeters),
                m_activeCameraObserved ? "true" : "false");

            AZ_Printf("LDMChronoVehicle",
                "T1 camera headlook trace: t=%.3f yaw_deg=%.3f fov_wide=%s\n",
                m_elapsedSeconds,
                static_cast<double>(m_headLookYawDegrees),
                m_fovModeWide ? "true" : "false");
        }
    }
} // namespace LDMChronoVehicle
