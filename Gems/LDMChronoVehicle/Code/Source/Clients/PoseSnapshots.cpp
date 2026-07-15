#include "PoseSnapshots.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Socket/AzSocket.h>
#include <AzCore/std/algorithm.h>

#include <cmath>
#include <cstring>
#include <type_traits>

namespace LDMChronoVehicle
{
    static_assert(std::is_trivially_copyable_v<PoseSnapshotPacket>,
        "Snapshot packets are sent as raw bytes and must stay trivially copyable.");

    namespace
    {
        // Winsock constants; this Gem is Windows-only (ADR 0003).
        constexpr AZ::s32 AfInet = 2;
        constexpr AZ::s32 SockDgram = 2;
        constexpr AZ::s32 IpProtoUdp = 17;

        AZSOCKET OpenConnectedUdpSocket(AZ::u16 localPort, AZ::u16 remotePort)
        {
            const AZSOCKET sock = AZ::AzSock::Socket(AfInet, SockDgram, IpProtoUdp);
            if (!AZ::AzSock::IsAzSocketValid(sock))
            {
                AZ_Error("LDMChronoVehicle", false,
                    "Snapshot socket creation failed (%d).", sock);
                return -1;
            }

            AZ::AzSock::SetSocketOption(sock, AZ::AzSock::AzSocketOption::REUSEADDR, true);
            AZ::AzSock::SetSocketBlockingMode(sock, false);

            AZ::AzSock::AzSocketAddress localAddress;
            localAddress.SetAddress(SnapshotConfig::LoopbackAddress, localPort);
            if (AZ::AzSock::SocketErrorOccured(AZ::AzSock::Bind(sock, localAddress)))
            {
                AZ_Error("LDMChronoVehicle", false,
                    "Snapshot socket bind to port %u failed.", localPort);
                AZ::AzSock::CloseSocket(sock);
                return -1;
            }

            AZ::AzSock::AzSocketAddress remoteAddress;
            remoteAddress.SetAddress(SnapshotConfig::LoopbackAddress, remotePort);
            if (AZ::AzSock::SocketErrorOccured(AZ::AzSock::Connect(sock, remoteAddress)))
            {
                AZ_Error("LDMChronoVehicle", false,
                    "Snapshot socket connect to port %u failed.", remotePort);
                AZ::AzSock::CloseSocket(sock);
                return -1;
            }

            return sock;
        }
    } // namespace

    bool PoseSnapshotPacket::IsValid() const
    {
        const bool headerValid = m_magic == ExpectedMagic && m_version == ExpectedVersion;
        const bool payloadFinite =
            std::isfinite(m_simulationTimeSeconds) &&
            std::isfinite(m_positionX) && std::isfinite(m_positionY) && std::isfinite(m_positionZ) &&
            std::isfinite(m_rotationW) && std::isfinite(m_rotationX) &&
            std::isfinite(m_rotationY) && std::isfinite(m_rotationZ) &&
            std::isfinite(m_condition);
        const double rotationLengthSquared =
            m_rotationW * m_rotationW + m_rotationX * m_rotationX +
            m_rotationY * m_rotationY + m_rotationZ * m_rotationZ;
        const bool rotationValid = std::isfinite(rotationLengthSquared) &&
            rotationLengthSquared > 0.99 && rotationLengthSquared < 1.01;
        const bool conditionValid = m_condition >= 0.0f && m_condition <= 1.01f && m_conditionState <= 3;
        bool ammoValid = true;
        for (int i = 0; i < 6; ++i)
        {
            if (m_ammo[i] > 100 || m_equipped[i] > 1)
            {
                ammoValid = false;
                break;
            }
        }
        return headerValid && payloadFinite && rotationValid && conditionValid && ammoValid &&
            m_simulationTimeSeconds >= 0.0 && m_vehicleId != InvalidVehicleId;
    }

    AZ::Transform PoseSnapshotPacket::GetPose() const
    {
        const AZ::Quaternion rotation(
            static_cast<float>(m_rotationX),
            static_cast<float>(m_rotationY),
            static_cast<float>(m_rotationZ),
            static_cast<float>(m_rotationW));
        const AZ::Vector3 position(
            static_cast<float>(m_positionX),
            static_cast<float>(m_positionY),
            static_cast<float>(m_positionZ));
        return AZ::Transform::CreateFromQuaternionAndTranslation(
            rotation.GetNormalized(), position);
    }

    PoseSnapshotPublisher::PoseSnapshotPublisher()
    {
        AZ::AzSock::Startup();
        m_socket = OpenConnectedUdpSocket(SnapshotConfig::ServerPort, SnapshotConfig::ClientPort);
    }

    PoseSnapshotPublisher::~PoseSnapshotPublisher()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
        }
        AZ::AzSock::Cleanup();
    }

    bool PoseSnapshotPublisher::IsReady() const
    {
        return AZ::AzSock::IsAzSocketValid(m_socket);
    }

    void PoseSnapshotPublisher::Publish(double simulationTimeSeconds, VehicleId vehicleId,
        const AZ::Transform& pose, AZ::u32 terrainChecksum,
        const AZ::u32 ammo[6], const AZ::u8 equipped[6], AZ::u32 conditionState, float condition)
    {
        if (!IsReady())
        {
            return;
        }

        PoseSnapshotPacket packet;
        packet.m_simulationTimeSeconds = simulationTimeSeconds;
        packet.m_vehicleId = vehicleId;
        const AZ::Vector3 position = pose.GetTranslation();
        packet.m_positionX = static_cast<double>(position.GetX());
        packet.m_positionY = static_cast<double>(position.GetY());
        packet.m_positionZ = static_cast<double>(position.GetZ());
        const AZ::Quaternion rotation = pose.GetRotation();
        packet.m_rotationW = static_cast<double>(rotation.GetW());
        packet.m_rotationX = static_cast<double>(rotation.GetX());
        packet.m_rotationY = static_cast<double>(rotation.GetY());
        packet.m_rotationZ = static_cast<double>(rotation.GetZ());
        packet.m_terrainChecksum = terrainChecksum;
        for (int i = 0; i < 6; ++i)
        {
            packet.m_ammo[i] = ammo[i];
            packet.m_equipped[i] = equipped[i];
        }
        packet.m_conditionState = conditionState;
        packet.m_condition = condition;

        const AZ::s32 result = AZ::AzSock::Send(m_socket,
            reinterpret_cast<const char*>(&packet), sizeof(packet), 0);
        if (!AZ::AzSock::SocketErrorOccured(result))
        {
            ++m_sentCount;
        }
    }

    AZ::u64 PoseSnapshotPublisher::GetSentCount() const
    {
        return m_sentCount;
    }

    PoseSnapshotReceiver::PoseSnapshotReceiver()
    {
        AZ::AzSock::Startup();
        m_socket = OpenConnectedUdpSocket(SnapshotConfig::ClientPort, SnapshotConfig::ServerPort);
    }

    PoseSnapshotReceiver::~PoseSnapshotReceiver()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
        }
        AZ::AzSock::Cleanup();
    }

    bool PoseSnapshotReceiver::IsReady() const
    {
        return AZ::AzSock::IsAzSocketValid(m_socket);
    }

    bool PoseSnapshotReceiver::ReceiveLatest(PoseSnapshotBatch& outPackets)
    {
        if (!IsReady())
        {
            return false;
        }

        outPackets.clear();
        for (;;)
        {
            PoseSnapshotPacket packet;
            const AZ::s32 received = AZ::AzSock::Recv(m_socket,
                reinterpret_cast<char*>(&packet), sizeof(packet), 0);
            if (AZ::AzSock::SocketErrorOccured(received))
            {
                break; // drained (or transient loopback reset; retried next tick)
            }

            if (received != static_cast<AZ::s32>(sizeof(packet)) || !packet.IsValid())
            {
                ++m_rejectedCount;
                AZ_Error("LDMChronoVehicle", false,
                    "Rejected malformed pose snapshot (%d bytes).", received);
                continue;
            }

            ++m_receivedCount;
            auto existing = AZStd::find_if(outPackets.begin(), outPackets.end(),
                [vehicleId = packet.m_vehicleId](const PoseSnapshotPacket& candidate)
                {
                    return candidate.m_vehicleId == vehicleId;
                });
            if (existing == outPackets.end())
            {
                if (outPackets.size() < outPackets.capacity())
                {
                    outPackets.push_back(packet);
                }
            }
            else if (packet.m_simulationTimeSeconds >= existing->m_simulationTimeSeconds)
            {
                *existing = packet;
            }
        }

        return !outPackets.empty();
    }

    AZ::u64 PoseSnapshotReceiver::GetReceivedCount() const
    {
        return m_receivedCount;
    }

    AZ::u64 PoseSnapshotReceiver::GetRejectedCount() const
    {
        return m_rejectedCount;
    }

    bool VehicleInputPacket::IsValid() const
    {
        const bool headerValid = m_magic == ExpectedMagic && m_version == ExpectedVersion;
        const bool payloadFinite =
            std::isfinite(m_simulationTimeSeconds) &&
            std::isfinite(m_steering) && std::isfinite(m_throttle) && std::isfinite(m_braking);
        const bool valuesInBounds =
            m_steering >= -1.05 && m_steering <= 1.05 &&
            m_throttle >= 0.0 && m_throttle <= 1.05 &&
            m_braking >= 0.0 && m_braking <= 1.05 &&
            m_driveMode >= -1 && m_driveMode <= 1 &&
            m_fireTriggers[0] <= 1 && m_fireTriggers[1] <= 1 &&
            m_fireTriggers[2] <= 1 && m_fireTriggers[3] <= 1 &&
            m_fireTriggers[4] <= 1 && m_fireTriggers[5] <= 1;
        return headerValid && payloadFinite && valuesInBounds &&
            m_simulationTimeSeconds >= 0.0 && m_vehicleId != InvalidVehicleId;
    }

    VehicleInputPublisher::VehicleInputPublisher()
    {
        AZ::AzSock::Startup();
        m_socket = OpenConnectedUdpSocket(InputConfig::ClientPort, InputConfig::ServerPort);
    }

    VehicleInputPublisher::~VehicleInputPublisher()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
            m_socket = -1;
        }
        AZ::AzSock::Cleanup();
    }

    bool VehicleInputPublisher::IsReady() const
    {
        return AZ::AzSock::IsAzSocketValid(m_socket);
    }

    void VehicleInputPublisher::Publish(double simulationTimeSeconds, VehicleId vehicleId,
        double steering, double throttle, double braking, bool handbrake, int driveMode, bool engineStarted,
        const bool fireTriggers[6])
    {
        if (!IsReady())
        {
            return;
        }

        VehicleInputPacket packet;
        packet.m_simulationTimeSeconds = simulationTimeSeconds;
        packet.m_vehicleId = vehicleId;
        packet.m_steering = steering;
        packet.m_throttle = throttle;
        packet.m_braking = braking;
        packet.m_handbrake = handbrake ? 1 : 0;
        packet.m_driveMode = driveMode;
        packet.m_engineStarted = engineStarted ? 1 : 0;
        for (int i = 0; i < 6; ++i)
        {
            packet.m_fireTriggers[i] = fireTriggers[i] ? 1 : 0;
        }

        const AZ::s32 sent = AZ::AzSock::Send(m_socket,
            reinterpret_cast<const char*>(&packet), sizeof(packet), 0);
        if (sent == static_cast<AZ::s32>(sizeof(packet)))
        {
            ++m_sentCount;
        }
    }

    AZ::u64 VehicleInputPublisher::GetSentCount() const
    {
        return m_sentCount;
    }

    VehicleInputReceiver::VehicleInputReceiver()
    {
        AZ::AzSock::Startup();
        m_socket = OpenConnectedUdpSocket(InputConfig::ServerPort, InputConfig::ClientPort);
    }

    VehicleInputReceiver::~VehicleInputReceiver()
    {
        if (AZ::AzSock::IsAzSocketValid(m_socket))
        {
            AZ::AzSock::CloseSocket(m_socket);
            m_socket = -1;
        }
        AZ::AzSock::Cleanup();
    }

    bool VehicleInputReceiver::IsReady() const
    {
        return AZ::AzSock::IsAzSocketValid(m_socket);
    }

    bool VehicleInputReceiver::ReceiveLatest(VehicleId vehicleId, VehicleInputPacket& outPacket)
    {
        if (!IsReady())
        {
            return false;
        }

        bool found = false;
        while (true)
        {
            VehicleInputPacket packet;
            const AZ::s32 received = AZ::AzSock::Recv(m_socket,
                reinterpret_cast<char*>(&packet), sizeof(packet), 0);
            if (AZ::AzSock::SocketErrorOccured(received))
            {
                break; // drained
            }

            if (received != static_cast<AZ::s32>(sizeof(packet)) || !packet.IsValid())
            {
                ++m_rejectedCount;
                continue;
            }

            if (packet.m_vehicleId == vehicleId)
            {
                ++m_receivedCount;
                if (!found || packet.m_simulationTimeSeconds >= outPacket.m_simulationTimeSeconds)
                {
                    outPacket = packet;
                    found = true;
                }
            }
        }

        return found;
    }

    AZ::u64 VehicleInputReceiver::GetReceivedCount() const
    {
        return m_receivedCount;
    }

    AZ::u64 VehicleInputReceiver::GetRejectedCount() const
    {
        return m_rejectedCount;
    }
} // namespace LDMChronoVehicle
