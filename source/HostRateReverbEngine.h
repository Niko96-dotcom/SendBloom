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
        core.setParameters (rt60Seconds, darkMix);
        const auto out = core.processSample (input);
        return juce::jlimit (-4.0f, 4.0f, out);
    }

    void processBlock (const float* input,
                       float* output,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix) noexcept override
    {
        core.setParameters (rt60Seconds, darkMix);

        for (int i = 0; i < numSamples; ++i)
            output[i] = juce::jlimit (-4.0f, 4.0f, core.processSample (input[i]));
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
