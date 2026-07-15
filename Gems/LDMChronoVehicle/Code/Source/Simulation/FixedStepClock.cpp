#include "FixedStepClock.h"

#include <AzCore/Debug/Trace.h>

#include <cmath>

namespace LDMChronoVehicle
{
    FixedStepClock::FixedStepClock(double fixedStepSeconds, AZ::u32 maxCatchUpSteps)
        : m_fixedStepSeconds(fixedStepSeconds)
        , m_maxCatchUpSteps(maxCatchUpSteps)
    {
        AZ_Assert(std::isfinite(m_fixedStepSeconds) && m_fixedStepSeconds > 0.0,
            "The fixed simulation step must be finite and positive.");
        AZ_Assert(m_maxCatchUpSteps > 0, "At least one catch-up step must be allowed.");
    }

    FixedStepPlan FixedStepClock::Advance(double elapsedSeconds)
    {
        FixedStepPlan plan;
        plan.m_fixedStepSeconds = m_fixedStepSeconds;

        if (!std::isfinite(elapsedSeconds) || elapsedSeconds <= 0.0)
        {
            plan.m_accumulatorSeconds = m_accumulatorSeconds;
            return plan;
        }

        m_accumulatorSeconds += elapsedSeconds;
        constexpr double stepTolerance = 1e-12;
        while (plan.m_stepCount < m_maxCatchUpSteps &&
            m_accumulatorSeconds + stepTolerance >= m_fixedStepSeconds)
        {
            m_accumulatorSeconds -= m_fixedStepSeconds;
            ++plan.m_stepCount;
        }

        if (m_accumulatorSeconds + stepTolerance >= m_fixedStepSeconds)
        {
            const double droppedStepCount = std::floor((m_accumulatorSeconds + stepTolerance) / m_fixedStepSeconds);
            plan.m_droppedSeconds = droppedStepCount * m_fixedStepSeconds;
            m_accumulatorSeconds -= plan.m_droppedSeconds;
        }

        if (std::abs(m_accumulatorSeconds) <= stepTolerance)
        {
            m_accumulatorSeconds = 0.0;
        }

        plan.m_accumulatorSeconds = m_accumulatorSeconds;
        return plan;
    }

    void FixedStepClock::Reset()
    {
        m_accumulatorSeconds = 0.0;
    }

    double FixedStepClock::GetAccumulatorSeconds() const
    {
        return m_accumulatorSeconds;
    }
} // namespace LDMChronoVehicle
