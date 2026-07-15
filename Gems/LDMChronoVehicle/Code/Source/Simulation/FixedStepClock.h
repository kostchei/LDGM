#pragma once

#include <AzCore/base.h>

namespace LDMChronoVehicle
{
    struct FixedStepPlan
    {
        AZ::u32 m_stepCount = 0;
        double m_fixedStepSeconds = 0.0;
        double m_droppedSeconds = 0.0;
        double m_accumulatorSeconds = 0.0;
    };

    class FixedStepClock final
    {
    public:
        FixedStepClock(double fixedStepSeconds, AZ::u32 maxCatchUpSteps);

        FixedStepPlan Advance(double elapsedSeconds);
        void Reset();
        double GetAccumulatorSeconds() const;

    private:
        double m_fixedStepSeconds = 0.0;
        AZ::u32 m_maxCatchUpSteps = 0;
        double m_accumulatorSeconds = 0.0;
    };
} // namespace LDMChronoVehicle
