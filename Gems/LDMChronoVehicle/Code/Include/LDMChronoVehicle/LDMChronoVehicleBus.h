
#pragma once

#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace LDMChronoVehicle
{
    class LDMChronoVehicleRequests
    {
    public:
        AZ_RTTI(LDMChronoVehicleRequests, LDMChronoVehicleRequestsTypeId);
        virtual ~LDMChronoVehicleRequests() = default;
        // Put your public methods here
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
