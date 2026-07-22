#include "TerrainFixtureConfig.h"

#include <AzCore/Math/Crc.h>
#include <AzCore/std/string/string.h>

#include <chrono/physics/ChContactMaterialNSC.h>
#include <chrono/physics/ChSystem.h>
#include <chrono_vehicle/terrain/RigidTerrain.h>

#include <cmath>

namespace LDMChronoVehicle
{
    AZ::u32 CalculateTerrainFixtureChecksum()
    {
        const AZStd::string canonical = AZStd::string::format(
            "terrain-fixture|v%u|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f|%.6f",
            TerrainFixtureConfig::Version,
            TerrainFixtureConfig::LengthMeters,
            TerrainFixtureConfig::WidthMeters,
            TerrainFixtureConfig::SurfaceZMeters,
            TerrainFixtureConfig::ThicknessMeters,
            TerrainFixtureConfig::HardRoadFriction,
            TerrainFixtureConfig::BrokenRoadFriction,
            TerrainFixtureConfig::GravelFriction,
            TerrainFixtureConfig::FirmSandFriction,
            TerrainFixtureConfig::DeepSandFriction);
        return AZ::Crc32(canonical.c_str());
    }

    namespace TerrainFixtureConfig
    {
        int GetLaneIndex(double y)
        {
            double clampedY = std::max(-99.999, std::min(99.999, y));
            return static_cast<int>((clampedY + 100.0) / LaneWidthMeters);
        }

        double GetLaneFriction(double y)
        {
            int index = GetLaneIndex(y);
            switch (index)
            {
            case 0: return HardRoadFriction;
            case 1: return BrokenRoadFriction;
            case 2: return GravelFriction;
            case 3: return FirmSandFriction;
            case 4: return DeepSandFriction;
            default: return GravelFriction;
            }
        }

        const char* GetLaneName(double y)
        {
            int index = GetLaneIndex(y);
            switch (index)
            {
            case 0: return "Hard Road";
            case 1: return "Broken Road";
            case 2: return "Gravel";
            case 3: return "Firm Sand";
            case 4: return "Deep Sand";
            default: return "Gravel";
            }
        }
    }

    bool TerrainAlignmentMeasurement::Passes(double toleranceMeters) const
    {
        return std::isfinite(toleranceMeters) && toleranceMeters >= 0.0 &&
            m_originErrorMeters <= toleranceMeters &&
            m_surfaceHeightErrorMeters <= toleranceMeters &&
            m_renderedLengthErrorMeters <= toleranceMeters &&
            m_renderedWidthErrorMeters <= toleranceMeters;
    }

    TerrainAlignmentMeasurement MeasureTerrainAlignment(
        double o3deOriginXMeters, double o3deOriginYMeters,
        double o3deSurfaceZMeters, double o3deUniformScale)
    {
        TerrainAlignmentMeasurement result;
        result.m_originErrorMeters = std::hypot(
            o3deOriginXMeters,
            o3deOriginYMeters);
        result.m_surfaceHeightErrorMeters = std::abs(
            o3deSurfaceZMeters - TerrainFixtureConfig::SurfaceZMeters);
        result.m_renderedLengthErrorMeters = std::abs(
            TerrainFixtureConfig::O3deGroundModelSizeMeters *
                o3deUniformScale -
            TerrainFixtureConfig::LengthMeters);
        result.m_renderedWidthErrorMeters = std::abs(
            TerrainFixtureConfig::O3deGroundModelSizeMeters *
                o3deUniformScale -
            TerrainFixtureConfig::WidthMeters);
        return result;
    }

    TerrainFixture::TerrainFixture(chrono::ChSystem& system)
    {
        m_terrain = std::make_unique<chrono::vehicle::RigidTerrain>(&system);

        auto material = chrono_types::make_shared<chrono::ChContactMaterialNSC>();
        material->SetFriction(0.85f);
        material->SetRestitution(static_cast<float>(TerrainFixtureConfig::DefaultRestitution));

        // Full 2500m x 2500m physical ground patch centered at (0, 0, 0)
        m_terrain->AddPatch(material,
            chrono::ChCoordsysd(
                chrono::ChVector3d(0.0, 0.0, TerrainFixtureConfig::SurfaceZMeters), chrono::QUNIT),
            TerrainFixtureConfig::LengthMeters,
            TerrainFixtureConfig::WidthMeters,
            TerrainFixtureConfig::ThicknessMeters,
            false, 1.0, /*visualization*/ false);

        m_terrain->Initialize();
    }

    TerrainFixture::~TerrainFixture() = default;

    chrono::vehicle::RigidTerrain& TerrainFixture::GetTerrain()
    {
        return *m_terrain;
    }

    void TerrainFixture::Synchronize(double simulationTimeSeconds)
    {
        m_terrain->Synchronize(simulationTimeSeconds);
    }

    void TerrainFixture::Advance(double fixedStepSeconds)
    {
        m_terrain->Advance(fixedStepSeconds);
    }
} // namespace LDMChronoVehicle
