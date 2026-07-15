#include "PoseSnapshots.h"

#include <AzCore/Debug/Trace.h>
#include <AzCore/Math/Quaternion.h>
#include <AzCore/Math/Vector3.h>
#include <AzCore/Socket/AzSocket.h>

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
            std::isfinite(m_rotationY) && std::isfinite(m_rotationZ);
        return headerValid && payloadFinite && m_vehicleId != InvalidVehicleId;
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
        const AZ::Transform& pose, AZ::u32 terrainChecksum)
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

        // A send error simply means no client is listening yet (loopback
        // ICMP unreachable); the next publish re-arms the socket.
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

    bool PoseSnapshotReceiver::ReceiveLatest(PoseSnapshotPacket& outPacket)
    {
        if (!IsReady())
        {
            return false;
        }

        bool anyValid = false;
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
            outPacket = packet;
            anyValid = true;
        }

        return anyValid;
    }

    AZ::u64 PoseSnapshotReceiver::GetReceivedCount() const
    {
        return m_receivedCount;
    }

    AZ::u64 PoseSnapshotReceiver::GetRejectedCount() const
    {
        return m_rejectedCount;
    }
} // namespace LDMChronoVehicle
