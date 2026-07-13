#pragma once

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS

#include "IReverbEngine.h"
#include "SchroederTankCore.h"
#include <juce_dsp/juce_dsp.h>

namespace sendbloom
{

class HostRateReverbEngine : public IReverbEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept override
    {
        core.prepare (sampleRate, maxBlockSize);
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix) noexcept override
    {
        const auto out = core.processSample (input, rt60Seconds, darkMix);
        return juce::jlimit (-4.0f, 4.0f, out);
    }

    void reset() noexcept
    {
        core.reset();
    }

private:
    SchroederTankCore core;
};

} // namespace sendbloom

#endif
