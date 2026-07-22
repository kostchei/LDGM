#pragma once

#include <AzCore/Math/Vector3.h>
#include <AzCore/Math/Transform.h>
#include <AzCore/std/containers/vector.h>
#include <AzCore/std/string/string.h>

namespace LDMChronoVehicle
{
    enum class RaceState : AZ::u32
    {
        Countdown = 0,
        Racing = 1,
        Finished = 2,
        PlayerEliminated = 3
    };

    struct CompetitorRaceStatus
    {
        AZ::u64 m_vehicleId = 0;
        AZStd::string m_driverName;
        bool m_isPlayer = false;
        AZ::u32 m_lap = 1;
        size_t m_currentWaypointIndex = 0;
        float m_distanceToNextWaypoint = 0.0f;
        float m_progressScore = 0.0f;
        AZ::u32 m_rank = 1;
        bool m_isEliminated = false;
        float m_healthRatio = 1.0f;
    };

    class RaceManager final
    {
    public:
        RaceManager();
        ~RaceManager() = default;

        static constexpr AZ::u32 TotalLaps = 3;
        static constexpr float CountdownSeconds = 3.0f;

        void InitializeTrackCircuit();
        void RegisterCompetitor(AZ::u64 vehicleId, const AZStd::string& driverName, bool isPlayer);

        void UpdateRace(float dtSeconds, const AZStd::vector<std::pair<AZ::u64, AZ::Transform>>& vehiclePoses,
            const AZStd::vector<std::pair<AZ::u64, float>>& vehicleHealthRatios);

        RaceState GetRaceState() const { return m_state; }
        float GetCountdownRemainingSeconds() const { return m_countdownTimer; }
        AZ::u32 GetTotalCompetitors() const { return static_cast<AZ::u32>(m_competitors.size()); }

        const AZStd::vector<AZ::Vector3>& GetTrackWaypoints() const { return m_waypoints; }
        const AZStd::vector<CompetitorRaceStatus>& GetLeaderboard() const { return m_competitors; }
        CompetitorRaceStatus GetPlayerStatus() const;

    private:
        RaceState m_state = RaceState::Countdown;
        float m_countdownTimer = CountdownSeconds;
        AZStd::vector<AZ::Vector3> m_waypoints;
        AZStd::vector<CompetitorRaceStatus> m_competitors;
    };
} // namespace LDMChronoVehicle
