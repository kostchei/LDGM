
#pragma once

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <LDMChronoVehicle/LDMChronoVehicleTypes.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>
#include <AzCore/Math/Transform.h>

namespace LDMChronoVehicle
{
    class LDMChronoVehicleRequests
    {
    public:
        AZ_RTTI(LDMChronoVehicleRequests, LDMChronoVehicleRequestsTypeId);
        virtual ~LDMChronoVehicleRequests() = default;

        virtual VehicleReservationResult ReserveActiveVehicle(VehicleId vehicleId) = 0;
        virtual bool ReleaseActiveVehicle(VehicleId vehicleId) = 0;
        virtual SimulationTelemetry GetSimulationTelemetry() const = 0;

        // Returns the authoritative chassis pose of an active vehicle in the
        // shared Z-up world frame. False when this process is not
        // authoritative or the vehicle is not active.
        virtual bool GetActiveVehiclePose(VehicleId vehicleId, AZ::Transform& outPose) const = 0;
    };

    class LDMChronoVehicleBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using LDMChronoVehicleRequestBus = AZ::EBus<LDMChronoVehicleRequests, LDMChronoVehicleBusTraits>;
    using LDMChronoVehicleInterface = AZ::Interface<LDMChronoVehicleRequests>;

} // namespace LDMChronoVehicle
