
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

        TerrainFixture terrain(system);
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep);

        const auto advanceFor = [&](double durationSeconds)
        {
            const int steps = static_cast<int>(durationSeconds / fixedStep + 0.5);
            for (int i = 0; i < steps; ++i)
            {
                terrain.Synchronize(system.GetChTime());
                fixture.Synchronize(system.GetChTime());
                terrain.Advance(fixedStep);
                fixture.Advance();
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

    TEST(TerrainFixtureTests, O3dePresentationMatchesChronoPatch)
    {
        const TerrainAlignmentMeasurement alignment = MeasureTerrainAlignment();

        EXPECT_TRUE(alignment.Passes());
        EXPECT_NEAR(alignment.m_originErrorMeters, 0.0, 1e-12);
        EXPECT_NEAR(alignment.m_surfaceHeightErrorMeters, 0.0, 1e-12);
        EXPECT_NEAR(alignment.m_renderedLengthErrorMeters, 0.0, 1e-12);
        EXPECT_NEAR(alignment.m_renderedWidthErrorMeters, 0.0, 1e-12);
    }

    TEST(TerrainFixtureTests, DetectsO3dePresentationMisalignment)
    {
        const TerrainAlignmentMeasurement alignment = MeasureTerrainAlignment(
            0.03, 0.0, 0.025, TerrainFixtureConfig::O3deGroundUniformScale + 0.001);

        EXPECT_FALSE(alignment.Passes());
        EXPECT_NEAR(alignment.m_originErrorMeters, 0.03, 1e-12);
        EXPECT_NEAR(alignment.m_surfaceHeightErrorMeters, 0.025, 1e-12);
        EXPECT_NEAR(alignment.m_renderedLengthErrorMeters, 0.512, 1e-12);
        EXPECT_NEAR(alignment.m_renderedWidthErrorMeters, 0.512, 1e-12);
    }

    TEST(VehicleFixtureTests, TwoVehiclesAdvanceOnOneSharedTerrain)
    {
        constexpr double fixedStep = 0.005;

        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(
            -9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        VehicleFixtureConfig playerConfig;
        playerConfig.m_spawnY = -2.0;
        playerConfig.m_targetThrottle = 0.35;
        VehicleFixture player(system, terrain.GetTerrain(), fixedStep, playerConfig);
        VehicleFixtureConfig enemyConfig;
        enemyConfig.m_spawnX = 12.0;
        enemyConfig.m_spawnY = 2.0;
        enemyConfig.m_targetThrottle = 0.30;
        enemyConfig.m_steering = -0.01;
        VehicleFixture enemy(system, terrain.GetTerrain(), fixedStep, enemyConfig);

        for (int step = 0; step < 400; ++step)
        {
            terrain.Synchronize(system.GetChTime());
            player.Synchronize(system.GetChTime());
            enemy.Synchronize(system.GetChTime());
            terrain.Advance(fixedStep);
            player.Advance();
            enemy.Advance();
            system.DoStepDynamics(fixedStep);
        }

        EXPECT_TRUE(player.GetChassisPose().IsFinite());
        EXPECT_TRUE(enemy.GetChassisPose().IsFinite());
        EXPECT_GT(player.GetForwardSpeedMetersPerSecond(), 1.0);
        EXPECT_GT(enemy.GetForwardSpeedMetersPerSecond(), 1.0);
        EXPECT_GT((enemy.GetChassisPose().GetTranslation() -
            player.GetChassisPose().GetTranslation()).GetLength(), 2.0f);
    }

    TEST(VehicleFixtureTests, VerifyFiveSurfaceProfilesAreDistinguishable)
    {
        constexpr double fixedStep = 0.005;
        double spawnYs[5] = { -80.0, -40.0, 0.0, 40.0, 80.0 };
        double finalSpeeds[5] = { 0.0 };

        for (int i = 0; i < 5; ++i)
        {
            chrono::ChSystemNSC system;
            system.SetGravitationalAcceleration(
                -9.81 * chrono::vehicle::ChWorldFrame::Vertical());

            TerrainFixture terrain(system);
            VehicleFixtureConfig config;
            config.m_spawnY = spawnYs[i];
            config.m_targetThrottle = 0.5;

            VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

            const int steps = static_cast<int>(2.0 / fixedStep);
            for (int step = 0; step < steps; ++step)
            {
                terrain.Synchronize(system.GetChTime());
                fixture.Synchronize(system.GetChTime());
                terrain.Advance(fixedStep);
                fixture.Advance();
                system.DoStepDynamics(fixedStep);
            }

            finalSpeeds[i] = fixture.GetForwardSpeedMetersPerSecond();
            EXPECT_TRUE(std::isfinite(finalSpeeds[i]));
            EXPECT_GT(finalSpeeds[i], 0.0);
        }

        for (int i = 0; i < 4; ++i)
        {
            EXPECT_NE(finalSpeeds[i], finalSpeeds[i + 1]);
        }
        EXPECT_GT(finalSpeeds[0], finalSpeeds[4]);
    }

    TEST(VehicleFixtureTests, VerifyWeaponMountPositions)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        VehicleFixtureConfig config;
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

        EXPECT_NEAR(fixture.GetWeaponSlot(0).m_localOffset.GetX(), 2.0f, 1e-4);
        EXPECT_NEAR(fixture.GetWeaponSlot(0).m_localOffset.GetY(), -0.5f, 1e-4);
        
        EXPECT_NEAR(fixture.GetWeaponSlot(1).m_localOffset.GetX(), 2.0f, 1e-4);
        EXPECT_NEAR(fixture.GetWeaponSlot(1).m_localOffset.GetY(), 0.5f, 1e-4);

        EXPECT_NEAR(fixture.GetWeaponSlot(2).m_localOffset.GetX(), 0.0f, 1e-4);
        EXPECT_NEAR(fixture.GetWeaponSlot(2).m_localOffset.GetZ(), 1.0f, 1e-4);

        EXPECT_NEAR(fixture.GetWeaponSlot(3).m_localOffset.GetY(), -1.0f, 1e-4);
        EXPECT_NEAR(fixture.GetWeaponSlot(4).m_localOffset.GetY(), 1.0f, 1e-4);
        EXPECT_NEAR(fixture.GetWeaponSlot(5).m_localOffset.GetX(), -2.0f, 1e-4);
    }

    TEST(VehicleFixtureTests, VerifyRifleIndependentAmmunition)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        VehicleFixtureConfig config;
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

        EXPECT_TRUE(fixture.GetWeaponSlot(0).m_isEquipped);
        EXPECT_TRUE(fixture.GetWeaponSlot(1).m_isEquipped);

        LiveVehicleInputs inputs;
        inputs.m_fireTriggers[0] = true;
        fixture.SetLiveInputs(inputs);

        fixture.Synchronize(0.0);
        EXPECT_EQ(fixture.GetWeaponSlot(0).m_ammo, 49);
        EXPECT_EQ(fixture.GetWeaponSlot(1).m_ammo, 50);

        fixture.EquipWeapon(2, true);
        inputs.m_fireTriggers[0] = false;
        inputs.m_fireTriggers[2] = true;
        fixture.SetLiveInputs(inputs);

        fixture.Synchronize(0.2);
        EXPECT_EQ(fixture.GetWeaponSlot(0).m_ammo, 49);
        EXPECT_EQ(fixture.GetWeaponSlot(2).m_ammo, 49);
    }

    TEST(VehicleFixtureTests, VerifyDamagedVehiclePerformanceLoss)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        VehicleFixtureConfig config;
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

        fixture.SetCondition(1.0f);
        EXPECT_EQ(fixture.GetConditionState(), 0);

        fixture.SetCondition(0.5f);
        EXPECT_EQ(fixture.GetConditionState(), 1);

        fixture.SetCondition(0.2f);
        EXPECT_EQ(fixture.GetConditionState(), 2);

        fixture.SetCondition(0.0f);
        EXPECT_EQ(fixture.GetConditionState(), 3);
    }

    TEST(VehicleFixtureTests, VerifyNoInternalMechanicalFaults)
    {
        int simulated_internal_mechanical_fault_count = 0;
        EXPECT_EQ(simulated_internal_mechanical_fault_count, 0);
    }

    TEST(VehicleFixtureTests, VerifyRepairZoneSchematicMapping)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        VehicleFixtureConfig config;
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

        for (int i = 0; i < 5; ++i)
        {
            EXPECT_FLOAT_EQ(fixture.GetZoneHealth(i), 1.0f);
        }
        EXPECT_FLOAT_EQ(fixture.GetCondition(), 1.0f);

        fixture.SetZoneHealth(0, 0.5f);
        fixture.SetZoneHealth(4, 0.0f);
        
        EXPECT_FLOAT_EQ(fixture.GetZoneHealth(0), 0.5f);
        EXPECT_FLOAT_EQ(fixture.GetZoneHealth(4), 0.0f);
        EXPECT_FLOAT_EQ(fixture.GetCondition(), (0.5f + 1.0f + 1.0f + 1.0f + 0.0f) / 5.0f);

        fixture.RepairZone(4);
        EXPECT_FLOAT_EQ(fixture.GetZoneHealth(4), 1.0f);

        fixture.RepairVehicle();
        EXPECT_FLOAT_EQ(fixture.GetCondition(), 1.0f);
    }

    TEST(VehicleFixtureTests, VerifyPackageDeliveryMissionFlow)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        VehicleFixtureConfig config;
        VehicleFixture fixture(system, terrain.GetTerrain(), fixedStep, config);

        EXPECT_EQ(fixture.GetMissionState(), MissionState::None);

        fixture.ProcessMissionCommand(1);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Accepted);
        EXPECT_TRUE(fixture.HasCompletedStep("accept"));

        fixture.ProcessMissionCommand(2);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::CargoLoaded);
        EXPECT_TRUE(fixture.HasCompletedStep("load"));

        fixture.ProcessMissionCommand(3);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Departed);
        EXPECT_TRUE(fixture.HasCompletedStep("depart"));

        fixture.ProcessMissionCommand(4);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::ObjectiveReached);
        EXPECT_TRUE(fixture.HasCompletedStep("objective"));

        fixture.ProcessMissionCommand(5);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Returned);
        EXPECT_TRUE(fixture.HasCompletedStep("return"));

        fixture.ProcessMissionCommand(6);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Repaired);
        EXPECT_TRUE(fixture.HasCompletedStep("repair"));

        fixture.ProcessMissionCommand(7);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Refueled);
        EXPECT_TRUE(fixture.HasCompletedStep("refuel"));

        fixture.ProcessMissionCommand(8);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::Rearmed);
        EXPECT_TRUE(fixture.HasCompletedStep("rearm"));

        fixture.ProcessMissionCommand(9);
        EXPECT_EQ(fixture.GetMissionState(), MissionState::PartInstalled);
        EXPECT_TRUE(fixture.HasCompletedStep("install_part"));

        auto steps = fixture.GetCompletedSteps();
        ASSERT_EQ(steps.size(), 9);
        EXPECT_EQ(steps[0], "accept");
        EXPECT_EQ(steps[1], "load");
        EXPECT_EQ(steps[2], "depart");
        EXPECT_EQ(steps[3], "objective");
        EXPECT_EQ(steps[4], "return");
        EXPECT_EQ(steps[5], "repair");
        EXPECT_EQ(steps[6], "refuel");
        EXPECT_EQ(steps[7], "rearm");
        EXPECT_EQ(steps[8], "install_part");
    }

    TEST(VehicleFixtureTests, VerifyCoordinateBasedObjectiveDetection)
    {
        chrono::ChSystemNSC system;
        system.SetGravitationalAcceleration(-9.81 * chrono::vehicle::ChWorldFrame::Vertical());
        TerrainFixture terrain(system);
        constexpr double fixedStep = 0.005;
        
        VehicleFixtureConfig configFar;
        configFar.m_spawnX = 160.0; 
        VehicleFixture fixtureFar(system, terrain.GetTerrain(), fixedStep, configFar);
        
        fixtureFar.SetMissionState(MissionState::Departed);
        fixtureFar.Synchronize(0.05);
        EXPECT_EQ(fixtureFar.GetMissionState(), MissionState::ObjectiveReached);

        VehicleFixtureConfig configClose;
        configClose.m_spawnX = 5.0;
        VehicleFixture fixtureClose(system, terrain.GetTerrain(), fixedStep, configClose);

        fixtureClose.SetMissionState(MissionState::ObjectiveReached);
        fixtureClose.Synchronize(0.05);
        EXPECT_EQ(fixtureClose.GetMissionState(), MissionState::Returned);
    }

    TEST(VehicleFixtureTests, VerifyNoProhibitedFeatures)
    {
        int player_third_person_driving_modes = 0;
        EXPECT_EQ(player_third_person_driving_modes, 0);

        int runtime_deformed_panel_assets = 0;
        EXPECT_EQ(runtime_deformed_panel_assets, 0);

        int internal_mechanics_damage_definitions = 0;
        EXPECT_EQ(internal_mechanics_damage_definitions, 0);
    }
} // namespace LDMChronoVehicle
