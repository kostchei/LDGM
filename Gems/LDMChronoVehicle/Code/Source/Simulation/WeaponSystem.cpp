#include "WeaponSystem.h"

#include <AzCore/Math/MathUtils.h>

namespace LDMChronoVehicle
{
    WeaponFiringResult WeaponSystem::FireWeaponSlot(AZ::u64 vehicleId, const AZ::Transform& vehiclePose,
        const AZ::Vector3& mountOffset, AZ::u32& ammoCount)
    {
        WeaponFiringResult result;
        if (ammoCount == 0)
        {
            return result;
        }

        --ammoCount;
        result.m_fired = true;

        // Weapon fire direction is chassis forward (+X)
        const AZ::Vector3 forward = vehiclePose.GetBasisX().GetNormalized();
        const AZ::Vector3 muzzleWorldPos = vehiclePose * mountOffset;

        // 5.56 NATO initial velocity vector
        TracerProjectile projectile;
        projectile.m_position = muzzleWorldPos;
        projectile.m_velocity = forward * MuzzleVelocityMetersPerSec;
        projectile.m_lifeTimeSeconds = 1.5f; // Max 1.5s flight range (~1400m)
        projectile.m_shooterVehicleId = vehicleId;
        projectile.m_damage = 0.15f; // 15% structural zone damage per hit

        m_projectiles.push_back(projectile);

        // Linear recoil force opposing muzzle direction
        result.m_recoilForce = -forward * RecoilImpulseNewtonSec;
        result.m_cameraAttentionKick = AttentionKickDegrees;

        return result;
    }

    void WeaponSystem::AdvanceProjectiles(float dtSeconds)
    {
        constexpr AZ::Vector3 gravity(0.0f, 0.0f, -9.81f);

        for (auto it = m_projectiles.begin(); it != m_projectiles.end();)
        {
            it->m_lifeTimeSeconds -= dtSeconds;
            if (it->m_lifeTimeSeconds <= 0.0f)
            {
                it = m_projectiles.erase(it);
                continue;
            }

            // Parabolic gravity flight step
            it->m_velocity += gravity * dtSeconds;
            it->m_position += it->m_velocity * dtSeconds;
            ++it;
        }
    }
} // namespace LDMChronoVehicle
