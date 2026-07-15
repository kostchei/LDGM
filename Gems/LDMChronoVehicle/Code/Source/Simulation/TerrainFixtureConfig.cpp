#include "TerrainFixtureConfig.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>

namespace LDMChronoVehicle
{
    AZ::u32 CalculateTerrainFixtureChecksum()
    {
        const AZStd::string canonical = AZStd::string::format(
            "terrain-fixture|v%u|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f",
            TerrainFixtureConfig::Version,
            TerrainFixtureConfig::LengthMeters,
            TerrainFixtureConfig::WidthMeters,
            TerrainFixtureConfig::SurfaceZMeters,
            TerrainFixtureConfig::ThicknessMeters,
            TerrainFixtureConfig::Friction,
            TerrainFixtureConfig::Restitution);
        return AZ::Crc32(canonical.c_str());
    }
} // namespace LDMChronoVehicle
