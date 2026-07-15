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
        auto material = chrono_types::make_shared<chrono::ChContactMaterialNSC>();
        material->SetFriction(static_cast<float>(TerrainFixtureConfig::Friction));
        material->SetRestitution(static_cast<float>(TerrainFixtureConfig::Restitution));

        m_terrain = std::make_unique<chrono::vehicle::RigidTerrain>(&system);
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
