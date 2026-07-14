
#pragma once

#include <LDGM/LDGMTypeIds.h>

#include <AzCore/EBus/EBus.h>
#include <AzCore/Interface/Interface.h>

namespace LDGM
{
    class LDGMRequests
    {
    public:
        AZ_RTTI(LDGMRequests, LDGMRequestsTypeId);
        virtual ~LDGMRequests() = default;
        // Put your public methods here
    };

    class LDGMBusTraits
        : public AZ::EBusTraits
    {
    public:
        //////////////////////////////////////////////////////////////////////////
        // EBusTraits overrides
        static constexpr AZ::EBusHandlerPolicy HandlerPolicy = AZ::EBusHandlerPolicy::Single;
        static constexpr AZ::EBusAddressPolicy AddressPolicy = AZ::EBusAddressPolicy::Single;
        //////////////////////////////////////////////////////////////////////////
    };

    using LDGMRequestBus = AZ::EBus<LDGMRequests, LDGMBusTraits>;
    using LDGMInterface = AZ::Interface<LDGMRequests>;

} // namespace LDGM
