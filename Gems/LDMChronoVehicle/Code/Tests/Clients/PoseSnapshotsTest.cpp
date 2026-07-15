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
} // namespace LDMChronoVehicle
