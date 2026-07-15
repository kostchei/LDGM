#pragma once

#include <AzCore/Math/Transform.h>
#include <AzCore/base.h>
#include <AzCore/std/containers/fixed_vector.h>

#include <LDMChronoVehicle/LDMChronoVehicleTypes.h>

namespace LDMChronoVehicle
{
    // T0 spike snapshot channel (deliverable: headless authoritative host
    // sending snapshots to one client). Connected UDP over loopback with
    // fixed ports because AzSock exposes no sendto/recvfrom; both ends are
    // Windows x64 little-endian, so packets are trivially copyable structs.
    // T5 replaces this with interest-managed snapshots on a real transport.
    namespace SnapshotConfig
    {
        inline constexpr const char* LoopbackAddress = "127.0.0.1";
        inline constexpr AZ::u16 ServerPort = 33455;
        inline constexpr AZ::u16 ClientPort = 33456;
        inline constexpr double PublishIntervalSeconds = 0.05; // 20 Hz
    } // namespace SnapshotConfig

    struct PoseSnapshotPacket
    {
        static constexpr AZ::u32 ExpectedMagic = 0x4C44474D; // 'LDGM'
        static constexpr AZ::u32 ExpectedVersion = 1;

        AZ::u32 m_magic = ExpectedMagic;
        AZ::u32 m_version = ExpectedVersion;
        double m_simulationTimeSeconds = 0.0;
        AZ::u64 m_vehicleId = InvalidVehicleId;
        double m_positionX = 0.0;
        double m_positionY = 0.0;
        double m_positionZ = 0.0;
        double m_rotationW = 1.0;
        double m_rotationX = 0.0;
        double m_rotationY = 0.0;
        double m_rotationZ = 0.0;
        AZ::u32 m_terrainChecksum = 0;
        AZ::u32 m_ammo[6] = { 50, 50, 50, 50, 50, 50 };
        AZ::u8 m_equipped[6] = { 1, 1, 0, 0, 0, 0 };
        AZ::u32 m_conditionState = 0;
        float m_condition = 1.0f;
        AZ::u32 m_missionState = 0;
        float m_fuelRatio = 1.0f;

        bool IsValid() const;
        AZ::Transform GetPose() const;
    };

    using PoseSnapshotBatch = AZStd::fixed_vector<PoseSnapshotPacket, MaxActiveVehicles>;

    class PoseSnapshotPublisher final
    {
    public:
        PoseSnapshotPublisher();
        ~PoseSnapshotPublisher();

        bool IsReady() const;
        void Publish(double simulationTimeSeconds, VehicleId vehicleId,
            const AZ::Transform& pose, AZ::u32 terrainChecksum,
            const AZ::u32 ammo[6], const AZ::u8 equipped[6], AZ::u32 conditionState, float condition,
            AZ::u32 missionState, float fuelRatio);
        AZ::u64 GetSentCount() const;

    private:
        AZ::s32 m_socket = -1; // AZSOCKET
        AZ::u64 m_sentCount = 0;
    };

    class PoseSnapshotReceiver final
    {
    public:
        PoseSnapshotReceiver();
        ~PoseSnapshotReceiver();

        bool IsReady() const;
        // Drains the socket and returns the newest packet for every vehicle
        // observed in this tick. This preserves the player and enemy packets
        // that are published back-to-back at 20 Hz.
        bool ReceiveLatest(PoseSnapshotBatch& outPackets);
        AZ::u64 GetReceivedCount() const;
        AZ::u64 GetRejectedCount() const;

    private:
        AZ::s32 m_socket = -1; // AZSOCKET
        AZ::u64 m_receivedCount = 0;
        AZ::u64 m_rejectedCount = 0;
    };

    namespace InputConfig
    {
        inline constexpr AZ::u16 ServerPort = 33457;
        inline constexpr AZ::u16 ClientPort = 33458;
    } // namespace InputConfig

    struct VehicleInputPacket
    {
        static constexpr AZ::u32 ExpectedMagic = 0x4C444749; // 'LDGI'
        static constexpr AZ::u32 ExpectedVersion = 1;

        AZ::u32 m_magic = ExpectedMagic;
        AZ::u32 m_version = ExpectedVersion;
        double m_simulationTimeSeconds = 0.0;
        AZ::u64 m_vehicleId = InvalidVehicleId;

        double m_steering = 0.0;
        double m_throttle = 0.0;
        double m_braking = 0.0;
        AZ::u32 m_handbrake = 0;
        AZ::s32 m_driveMode = 1;
        AZ::u32 m_engineStarted = 1;
        AZ::u8 m_fireTriggers[6] = { 0, 0, 0, 0, 0, 0 };
        AZ::u32 m_missionCommand = 0;

        bool IsValid() const;
    };

    class VehicleInputPublisher final
    {
    public:
        VehicleInputPublisher();
        ~VehicleInputPublisher();

        bool IsReady() const;
        void Publish(double simulationTimeSeconds, VehicleId vehicleId,
            double steering, double throttle, double braking, bool handbrake, int driveMode, bool engineStarted,
            const bool fireTriggers[6], AZ::u32 missionCommand);
        AZ::u64 GetSentCount() const;

    private:
        AZ::s32 m_socket = -1; // AZSOCKET
        AZ::u64 m_sentCount = 0;
    };

    class VehicleInputReceiver final
    {
    public:
        VehicleInputReceiver();
        ~VehicleInputReceiver();

        bool IsReady() const;
        bool ReceiveLatest(VehicleId vehicleId, VehicleInputPacket& outPacket);
        AZ::u64 GetReceivedCount() const;
        AZ::u64 GetRejectedCount() const;

    private:
        AZ::s32 m_socket = -1; // AZSOCKET
        AZ::u64 m_receivedCount = 0;
        AZ::u64 m_rejectedCount = 0;
    };
} // namespace LDMChronoVehicle
