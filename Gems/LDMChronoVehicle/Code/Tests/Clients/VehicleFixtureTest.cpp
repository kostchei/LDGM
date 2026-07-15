
#include <AzTest/AzTest.h>

#include <Simulation/TerrainFixtureConfig.h>
#include <Simulation/VehicleFixture.h>

#include <chrono/physics/ChSystemNSC.h>
#include <chrono_vehicle/ChWorldFrame.h>

#include <cmath>

namespace LDMChronoVehicle
{
    TEST(TerrainFixtureTests, ChecksumIsStableAndNonZero)
    {
        const AZ::u32 first = CalculateTerrainFixtureChecksum();
        const AZ::u32 second = CalculateTerrainFixtureChecksum();

        EXPECT_NE(first, 0u);
        EXPECT_EQ(first, second);
    }

    TEST(VehicleFixtureTests, HmmwvSettlesThenDrivesForwardOnSharedTerrain)
    {
        constexpr double fixedStep = 0.005;

        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(
            -9.81 * chrono::vehicle::ChWorldFrame::Vertical());

        VehicleFixture fixture(system, fixedStep);

        const auto advanceFor = [&](double durationSeconds)
        {
            const int steps = static_cast<int>(durationSeconds / fixedStep + 0.5);
            for (int i = 0; i < steps; ++i)
            {
                fixture.SynchronizeAndAdvance(system.GetChTime());
                system.DoStepDynamics(fixedStep);
            }
        };

        // Settle phase of the scripted driver: no inputs, vehicle drops onto
        // the terrain patch and comes to rest near the spawn point.
        advanceFor(0.75);
        const AZ::Transform settled = fixture.GetChassisPose();
        ASSERT_TRUE(settled.IsFinite());
        EXPECT_GT(settled.GetTranslation().GetZ(), 0.2f);
        EXPECT_LT(settled.GetTranslation().GetZ(), 1.5f);
        EXPECT_NEAR(settled.GetTranslation().GetX(), 0.0f, 0.5f);
        EXPECT_NEAR(settled.GetTranslation().GetY(), 0.0f, 0.5f);
        EXPECT_LT(std::abs(fixture.GetForwardSpeedMetersPerSecond()), 0.5);

        // Throttle ramp plus straight-line drive: the vehicle must move
        // forward (+X), stay roughly in lane and stay on the terrain (Z
        // band). The Pitman-arm steering linkage is asymmetric, so zero
        // steering input still produces a small lateral pull; bound the
        // drift relative to forward travel rather than by an absolute guess.
        advanceFor(1.75);
        const AZ::Transform driven = fixture.GetChassisPose();
        ASSERT_TRUE(driven.IsFinite());
        const float forwardTravel =
            driven.GetTranslation().GetX() - settled.GetTranslation().GetX();
        const float lateralDrift =
            std::abs(driven.GetTranslation().GetY() - settled.GetTranslation().GetY());
        EXPECT_GT(forwardTravel, 1.0f);
        EXPECT_LT(lateralDrift, 0.25f * forwardTravel);
        EXPECT_GT(driven.GetTranslation().GetZ(), 0.2f);
        EXPECT_LT(driven.GetTranslation().GetZ(), 1.5f);
        EXPECT_GT(fixture.GetForwardSpeedMetersPerSecond(), 1.0);
    }
} // namespace LDMChronoVehicle
