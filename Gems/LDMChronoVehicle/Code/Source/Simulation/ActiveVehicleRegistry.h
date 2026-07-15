#pragma once

#include <AzCore/base.h>
#include <AzCore/std/containers/array.h>

namespace LDMChronoVehicle
{
    using VehicleId = AZ::u64;
    inline constexpr VehicleId InvalidVehicleId = 0;

    enum class VehicleReservationResult
    {
        Reserved,
        AlreadyReserved,
        CapacityReached,
        InvalidId
    };

    class ActiveVehicleRegistry final
    {
    public:
        static constexpr size_t MaxActiveVehicles = 30;

        VehicleReservationResult Reserve(VehicleId vehicleId);
        bool Release(VehicleId vehicleId);
        bool Contains(VehicleId vehicleId) const;
        size_t GetActiveCount() const;

    private:
        AZStd::array<VehicleId, MaxActiveVehicles> m_activeVehicleIds{};
        size_t m_activeCount = 0;
    };
} // namespace LDMChronoVehicle
