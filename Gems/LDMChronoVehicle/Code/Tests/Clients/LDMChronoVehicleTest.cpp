
#include <AzTest/AzTest.h>

#include <Simulation/ActiveVehicleRegistry.h>
#include <Simulation/FixedStepClock.h>

#include <limits>

AZ_UNIT_TEST_HOOK(DEFAULT_UNIT_TEST_ENV);

namespace LDMChronoVehicle
{
    TEST(ActiveVehicleRegistryTests, RejectsThirtyFirstUniqueVehicle)
    {
        ActiveVehicleRegistry registry;

        for (VehicleId id = 1; id <= ActiveVehicleRegistry::MaxActiveVehicles; ++id)
        {
            EXPECT_EQ(registry.Reserve(id), VehicleReservationResult::Reserved);
        }

        EXPECT_EQ(registry.GetActiveCount(), ActiveVehicleRegistry::MaxActiveVehicles);
        EXPECT_EQ(registry.Reserve(ActiveVehicleRegistry::MaxActiveVehicles + 1),
            VehicleReservationResult::CapacityReached);
        EXPECT_EQ(registry.GetActiveCount(), ActiveVehicleRegistry::MaxActiveVehicles);
    }

    TEST(ActiveVehicleRegistryTests, DuplicateAndReleaseOperationsAreIdempotent)
    {
        ActiveVehicleRegistry registry;
        constexpr VehicleId vehicleId = 42;

        EXPECT_EQ(registry.Reserve(vehicleId), VehicleReservationResult::Reserved);
        EXPECT_EQ(registry.Reserve(vehicleId), VehicleReservationResult::AlreadyReserved);
        EXPECT_EQ(registry.GetActiveCount(), 1);
        EXPECT_TRUE(registry.Release(vehicleId));
        EXPECT_FALSE(registry.Release(vehicleId));
        EXPECT_EQ(registry.GetActiveCount(), 0);
    }

    TEST(ActiveVehicleRegistryTests, RejectsInvalidVehicleId)
    {
        ActiveVehicleRegistry registry;

        EXPECT_EQ(registry.Reserve(InvalidVehicleId), VehicleReservationResult::InvalidId);
        EXPECT_FALSE(registry.Contains(InvalidVehicleId));
        EXPECT_FALSE(registry.Release(InvalidVehicleId));
    }

    TEST(FixedStepClockTests, AccumulatesPartialFramesIntoDeterministicSteps)
    {
        FixedStepClock clock(0.01, 4);

        FixedStepPlan plan = clock.Advance(0.006);
        EXPECT_EQ(plan.m_stepCount, 0);
        EXPECT_NEAR(plan.m_accumulatorSeconds, 0.006, 1e-12);

        plan = clock.Advance(0.004);
        EXPECT_EQ(plan.m_stepCount, 1);
        EXPECT_NEAR(plan.m_fixedStepSeconds, 0.01, 1e-12);
        EXPECT_NEAR(plan.m_accumulatorSeconds, 0.0, 1e-12);
        EXPECT_NEAR(plan.m_droppedSeconds, 0.0, 1e-12);
    }

    TEST(FixedStepClockTests, CapsCatchUpAndReportsDroppedWholeSteps)
    {
        FixedStepClock clock(0.01, 3);

        const FixedStepPlan plan = clock.Advance(0.075);

        EXPECT_EQ(plan.m_stepCount, 3);
        EXPECT_NEAR(plan.m_droppedSeconds, 0.04, 1e-12);
        EXPECT_NEAR(plan.m_accumulatorSeconds, 0.005, 1e-12);
    }

    TEST(FixedStepClockTests, IgnoresInvalidElapsedTimeAndCanReset)
    {
        FixedStepClock clock(0.01, 3);

        EXPECT_EQ(clock.Advance(-1.0).m_stepCount, 0);
        EXPECT_EQ(clock.Advance(std::numeric_limits<double>::infinity()).m_stepCount, 0);
        EXPECT_EQ(clock.Advance(std::numeric_limits<double>::quiet_NaN()).m_stepCount, 0);
        EXPECT_NEAR(clock.GetAccumulatorSeconds(), 0.0, 1e-12);

        EXPECT_EQ(clock.Advance(0.005).m_stepCount, 0);
        clock.Reset();
        EXPECT_NEAR(clock.GetAccumulatorSeconds(), 0.0, 1e-12);
    }
} // namespace LDMChronoVehicle
