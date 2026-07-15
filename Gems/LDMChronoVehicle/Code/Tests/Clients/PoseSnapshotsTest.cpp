#include <AzTest/AzTest.h>

#include <Clients/PoseSnapshots.h>

#include <limits>

namespace LDMChronoVehicle
{
    TEST(PoseSnapshotPacketTests, AcceptsFiniteUnitPoseForActiveVehicle)
    {
        PoseSnapshotPacket packet;
        packet.m_vehicleId = 1;
        packet.m_simulationTimeSeconds = 1.25;

        EXPECT_TRUE(packet.IsValid());
        EXPECT_TRUE(packet.GetPose().IsFinite());
    }

    TEST(PoseSnapshotPacketTests, RejectsInvalidTimeVehicleAndRotation)
    {
        PoseSnapshotPacket packet;
        packet.m_vehicleId = 1;

        packet.m_simulationTimeSeconds = -0.1;
        EXPECT_FALSE(packet.IsValid());

        packet.m_simulationTimeSeconds = 0.0;
        packet.m_vehicleId = InvalidVehicleId;
        EXPECT_FALSE(packet.IsValid());

        packet.m_vehicleId = 2;
        packet.m_rotationW = 0.0;
        EXPECT_FALSE(packet.IsValid());

        packet.m_rotationW = 1.0;
        packet.m_positionX = std::numeric_limits<double>::quiet_NaN();
        EXPECT_FALSE(packet.IsValid());
    }

    TEST(VehicleInputPacketTests, AcceptsValidInputs)
    {
        VehicleInputPacket packet;
        packet.m_vehicleId = 1;
        packet.m_simulationTimeSeconds = 0.5;
        packet.m_steering = 0.5;
        packet.m_throttle = 0.8;
        packet.m_braking = 0.0;
        packet.m_handbrake = 0;
        packet.m_driveMode = 1;
        packet.m_engineStarted = 1;

        EXPECT_TRUE(packet.IsValid());
    }

    TEST(VehicleInputPacketTests, RejectsOutOfBoundInputs)
    {
        VehicleInputPacket packet;
        packet.m_vehicleId = 1;
        packet.m_simulationTimeSeconds = 0.5;

        packet.m_steering = 1.5;
        EXPECT_FALSE(packet.IsValid());

        packet.m_steering = 0.0;
        packet.m_throttle = -0.1;
        EXPECT_FALSE(packet.IsValid());

        packet.m_throttle = 0.0;
        packet.m_driveMode = 2;
        EXPECT_FALSE(packet.IsValid());
    }
} // namespace LDMChronoVehicle
