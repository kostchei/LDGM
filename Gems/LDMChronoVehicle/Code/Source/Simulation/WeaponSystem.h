#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>

namespace LDMChronoVehicle
{
    struct TracerProjectile
    {
        AZ::Vector3 m_position = AZ::Vector3::CreateZero();
        AZ::Vector3 m_velocity = AZ::Vector3::CreateZero();
        float m_lifeTimeSeconds = 0.0f;
        AZ::u64 m_shooterVehicleId = 0;
        float m_damage = 0.15f; // 15% damage per 5.56 hit
    };

    struct WeaponFiringResult
    {
        bool m_fired = false;
        AZ::Vector3 m_recoilForce = AZ::Vector3::CreateZero();
        float m_cameraAttentionKick = 0.0f;
    };

    class WeaponSystem final
    {
    public:
        WeaponSystem() = default;
        ~WeaponSystem() = default;

        static constexpr float MuzzleVelocityMetersPerSec = 930.0f; // 5.56 NATO
        static constexpr float RecoilImpulseNewtonSec = 150.0f;
        static constexpr float AttentionKickDegrees = 0.8f;

        // Calculates firing recoil impulse and spawns a 5.56 tracer projectile
        WeaponFiringResult FireWeaponSlot(AZ::u64 vehicleId, const AZ::Transform& vehiclePose,
            const AZ::Vector3& mountOffset, AZ::u32& ammoCount);

        // Advances all active projectiles across fixed timestep dt
        void AdvanceProjectiles(float dtSeconds);

        // Returns active projectiles for rendering / hit processing
        const AZStd::vector<TracerProjectile>& GetActiveProjectiles() const { return m_projectiles; }
        void ClearProjectiles() { m_projectiles.clear(); }

    private:
        AZStd::vector<TracerProjectile> m_projectiles;
    };
} // namespace LDMChronoVehicle
