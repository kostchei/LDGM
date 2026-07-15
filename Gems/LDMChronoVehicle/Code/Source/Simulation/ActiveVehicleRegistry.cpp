#include "ActiveVehicleRegistry.h"

namespace LDMChronoVehicle
{
    VehicleReservationResult ActiveVehicleRegistry::Reserve(VehicleId vehicleId)
    {
        if (vehicleId == InvalidVehicleId)
        {
            return VehicleReservationResult::InvalidId;
        }

        if (Contains(vehicleId))
        {
            return VehicleReservationResult::AlreadyReserved;
        }

        if (m_activeCount == MaxActiveVehicles)
        {
            return VehicleReservationResult::CapacityReached;
        }

        m_activeVehicleIds[m_activeCount] = vehicleId;
        ++m_activeCount;
        return VehicleReservationResult::Reserved;
    }

    bool ActiveVehicleRegistry::Release(VehicleId vehicleId)
    {
        if (vehicleId == InvalidVehicleId)
        {
            return false;
        }

        for (size_t index = 0; index < m_activeCount; ++index)
        {
            if (m_activeVehicleIds[index] == vehicleId)
            {
                for (size_t nextIndex = index + 1; nextIndex < m_activeCount; ++nextIndex)
                {
                    m_activeVehicleIds[nextIndex - 1] = m_activeVehicleIds[nextIndex];
                }

                --m_activeCount;
                m_activeVehicleIds[m_activeCount] = InvalidVehicleId;
                return true;
            }
        }

        return false;
    }

    bool ActiveVehicleRegistry::Contains(VehicleId vehicleId) const
    {
        if (vehicleId == InvalidVehicleId)
        {
            return false;
        }

        for (size_t index = 0; index < m_activeCount; ++index)
        {
            if (m_activeVehicleIds[index] == vehicleId)
            {
                return true;
            }
        }

        return false;
    }

    size_t ActiveVehicleRegistry::GetActiveCount() const
    {
        return m_activeCount;
    }
} // namespace LDMChronoVehicle
