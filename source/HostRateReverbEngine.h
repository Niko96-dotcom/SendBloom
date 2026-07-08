#pragma once

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
                         float darkMix,
                         bool authenticColor) noexcept override
    {
        juce::ignoreUnused (authenticColor);
        const auto out = core.processSample (input, rt60Seconds, darkMix);
        return juce::jlimit (-4.0f, 4.0f, out);
    }

private:
    SchroederTankCore core;
};

} // namespace sendbloom
