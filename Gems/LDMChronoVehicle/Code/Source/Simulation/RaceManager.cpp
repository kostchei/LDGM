#include "RaceManager.h"

#include <algorithm>
#include <cmath>

namespace LDMChronoVehicle
{
    RaceManager::RaceManager()
    {
        InitializeTrackCircuit();
    }

    void RaceManager::InitializeTrackCircuit()
    {
        m_waypoints.clear();
        // Expanded 2.5 km Desert Canyon Circuit (12 spatial checkpoints)
        m_waypoints.push_back(AZ::Vector3(0.0f, 0.0f, 0.0f));       // 1. Start/Finish Gate (5-lane grid)
        m_waypoints.push_back(AZ::Vector3(400.0f, 20.0f, 0.0f));    // 2. High-speed Canyon Drag (80-100 km/h)
        m_waypoints.push_back(AZ::Vector3(750.0f, -50.0f, 0.0f));   // 3. Mesa Rock Barrier #1 Entry
        m_waypoints.push_back(AZ::Vector3(950.0f, -250.0f, 0.0f));  // 4. Banked Turn 1 Apex
        m_waypoints.push_back(AZ::Vector3(800.0f, -600.0f, 0.0f));  // 5. Back Canyon Straightaway
        m_waypoints.push_back(AZ::Vector3(400.0f, -750.0f, 0.0f));  // 6. Dune Pass Chicane Entry
        m_waypoints.push_back(AZ::Vector3(-100.0f, -700.0f, 0.0f)); // 7. Dune Pass Chicane Apex
        m_waypoints.push_back(AZ::Vector3(-450.0f, -500.0f, 0.0f)); // 8. Mesa Rock Barrier #2 Sight-line Cover
        m_waypoints.push_back(AZ::Vector3(-600.0f, -250.0f, 0.0f)); // 9. Sweeping Curve 2 Entry
        m_waypoints.push_back(AZ::Vector3(-400.0f, -50.0f, 0.0f));  // 10. Sweeping Curve 2 Apex
        m_waypoints.push_back(AZ::Vector3(-200.0f, 0.0f, 0.0f));   // 11. Final Straightaway
        m_waypoints.push_back(AZ::Vector3(-50.0f, 0.0f, 0.0f));    // 12. Final Sprint to Start/Finish
    }

    void RaceManager::RegisterCompetitor(AZ::u64 vehicleId, const AZStd::string& driverName, bool isPlayer)
    {
        CompetitorRaceStatus status;
        status.m_vehicleId = vehicleId;
        status.m_driverName = driverName;
        status.m_isPlayer = isPlayer;
        status.m_lap = 1;
        status.m_currentWaypointIndex = 0;
        status.m_healthRatio = 1.0f;
        m_competitors.push_back(status);
    }

    void RaceManager::UpdateRace(float dtSeconds, const AZStd::vector<std::pair<AZ::u64, AZ::Transform>>& vehiclePoses,
        const AZStd::vector<std::pair<AZ::u64, float>>& vehicleHealthRatios)
    {
        if (m_state == RaceState::Countdown)
        {
            m_countdownTimer -= dtSeconds;
            if (m_countdownTimer <= 0.0f)
            {
                m_countdownTimer = 0.0f;
                m_state = RaceState::Racing;
            }
            return;
        }

        if (m_state == RaceState::Finished || m_state == RaceState::PlayerEliminated)
        {
            return;
        }

        // Update each competitor's health, track position, and lap progression
        for (auto& competitor : m_competitors)
        {
            // Health update
            for (const auto& healthPair : vehicleHealthRatios)
            {
                if (healthPair.first == competitor.m_vehicleId)
                {
                    competitor.m_healthRatio = healthPair.second;
                    if (competitor.m_healthRatio <= 0.0f)
                    {
                        competitor.m_isEliminated = true;
                        if (competitor.m_isPlayer)
                        {
                            m_state = RaceState::PlayerEliminated;
                        }
                    }
                    break;
                }
            }

            if (competitor.m_isEliminated)
            {
                competitor.m_progressScore = -9999.0f;
                continue;
            }

            // Pose and Checkpoint progression
            for (const auto& posePair : vehiclePoses)
            {
                if (posePair.first == competitor.m_vehicleId)
                {
                    const AZ::Vector3 pos = posePair.second.GetTranslation();
                    const AZ::Vector3 nextWaypoint = m_waypoints[competitor.m_currentWaypointIndex % m_waypoints.size()];
                    AZ::Vector3 toNext = nextWaypoint - pos;
                    toNext.SetZ(0.0f);

                    competitor.m_distanceToNextWaypoint = toNext.GetLength();

                    // Checkpoint passage threshold (18 meters)
                    if (competitor.m_distanceToNextWaypoint < 18.0f)
                    {
                        competitor.m_currentWaypointIndex = (competitor.m_currentWaypointIndex + 1);
                        if (competitor.m_currentWaypointIndex >= m_waypoints.size() * competitor.m_lap)
                        {
                            competitor.m_lap++;
                            if (competitor.m_isPlayer && competitor.m_lap > TotalLaps)
                            {
                                m_state = RaceState::Finished;
                            }
                        }
                    }

                    // Progress score calculation: (Lap * 10000) + (Waypoint * 1000) - distanceToNext
                    competitor.m_progressScore = static_cast<float>(competitor.m_lap * 10000 + competitor.m_currentWaypointIndex * 1000)
                        - competitor.m_distanceToNextWaypoint;

                    break;
                }
            }
        }

        // Sort leaderboard positions based on progress score
        AZStd::sort(m_competitors.begin(), m_competitors.end(), [](const CompetitorRaceStatus& a, const CompetitorRaceStatus& b) {
            return a.m_progressScore > b.m_progressScore;
        });

        // Assign rank indices (1st, 2nd, 3rd, etc.)
        for (size_t i = 0; i < m_competitors.size(); ++i)
        {
            m_competitors[i].m_rank = static_cast<AZ::u32>(i + 1);
        }
    }

    CompetitorRaceStatus RaceManager::GetPlayerStatus() const
    {
        for (const auto& competitor : m_competitors)
        {
            if (competitor.m_isPlayer)
            {
                return competitor;
            }
        }
        CompetitorRaceStatus fallback;
        fallback.m_driverName = "Player";
        fallback.m_isPlayer = true;
        return fallback;
    }
} // namespace LDMChronoVehicle
