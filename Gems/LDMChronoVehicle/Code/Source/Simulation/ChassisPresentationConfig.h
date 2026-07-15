#pragma once

#include <AzCore/Math/Vector3.h>

namespace LDMChronoVehicle
{
    // Shared chassis geometry contract for the starter technical.
    //
    // The authoritative simulation (weapon fire origins) and the client
    // presentation (cockpit rig, weapon visuals, camera anchor) must agree on
    // these offsets, so they live in one header instead of being duplicated
    // per side. All values are in the chassis local frame: right-handed
    // Z-up, +X forward, +Y left, origin at the Chrono chassis reference
    // (~0.59 m above ground at rest ride height).
    namespace ChassisPresentationConfig
    {
        // Driver eye point: left seat, above the cabin floor, behind the dash.
        inline const AZ::Vector3 DriverEyeOffset{ 0.4f, 0.4f, 1.1f };

        // Camera near-clip distance in meters. Cockpit geometry (dash,
        // windshield pillars) sits closer than O3DE's default near plane.
        inline constexpr float CameraNearClipMeters = 0.05f;

        inline constexpr int WeaponMountCount = 6;

        // Mount order matches VehicleFixture::m_weaponSlots and the snapshot
        // ammo/equipped arrays: 0 Front Left, 1 Front Right, 2 Central (roof),
        // 3 Left Side, 4 Right Side, 5 Rear. The front rifles sit at cowl
        // height (z = 1.0) so their sight line clears the engine box and is
        // usable from the driver eye point.
        inline const AZ::Vector3 WeaponMountOffsets[WeaponMountCount] = {
            AZ::Vector3(2.0f, -0.5f, 1.0f),  // Front Left
            AZ::Vector3(2.0f, 0.5f, 1.0f),   // Front Right
            AZ::Vector3(0.0f, 0.0f, 1.0f),   // Central (roof)
            AZ::Vector3(0.0f, -1.0f, 0.5f),  // Left Side
            AZ::Vector3(0.0f, 1.0f, 0.5f),   // Right Side
            AZ::Vector3(-2.0f, 0.0f, 0.5f),  // Rear
        };
    } // namespace ChassisPresentationConfig
} // namespace LDMChronoVehicle
