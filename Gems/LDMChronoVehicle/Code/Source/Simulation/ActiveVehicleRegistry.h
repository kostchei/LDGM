#pragma once

#include <AzCore/std/containers/array.h>
#include <LDMChronoVehicle/LDMChronoVehicleTypes.h>

namespace LDMChronoVehicle
{
    class ActiveVehicleRegistry final
    {
    public:
        VehicleReservationResult Reserve(VehicleId vehicleId);
        bool Release(VehicleId vehicleId);
        bool Contains(VehicleId vehicleId) const;
        size_t GetActiveCount() const;

    private:
        AZStd::array<VehicleId, MaxActiveVehicles> m_activeVehicleIds{};
        size_t m_activeCount = 0;
    };
} // namespace LDMChronoVehicle
