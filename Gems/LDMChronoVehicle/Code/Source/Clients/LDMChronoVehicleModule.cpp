
#include <LDMChronoVehicle/LDMChronoVehicleTypeIds.h>
#include <LDMChronoVehicleModuleInterface.h>
#include "LDMChronoVehicleSystemComponent.h"

namespace LDMChronoVehicle
{
    class LDMChronoVehicleModule
        : public LDMChronoVehicleModuleInterface
    {
    public:
        AZ_RTTI(LDMChronoVehicleModule, LDMChronoVehicleModuleTypeId, LDMChronoVehicleModuleInterface);
        AZ_CLASS_ALLOCATOR(LDMChronoVehicleModule, AZ::SystemAllocator);
    };
}// namespace LDMChronoVehicle

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), LDMChronoVehicle::LDMChronoVehicleModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_LDMChronoVehicle, LDMChronoVehicle::LDMChronoVehicleModule)
#endif
