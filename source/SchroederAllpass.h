#pragma once

#include <juce_dsp/juce_dsp.h>

namespace sendbloom
{

class SchroederAllpass
{
public:
    void prepare (double sampleRate, int maxDelaySamples) noexcept
    {
        sr = sampleRate;

        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        delayLine.setMaximumDelayInSamples (maxDelaySamples);
        delayLine.prepare (spec);
        delayLine.reset();
    }

    void setDelay (float delaySamples) noexcept
    {
        delayLine.setDelay (delaySamples);
    }

    void setFeedback (float feedback) noexcept
    {
        g = feedback;
    }

    float processSample (float input) noexcept
    {
        const auto delayed = delayLine.popSample (0);
        const auto v = input + g * delayed;
        delayLine.pushSample (0, v);
        return delayed - g * v;
    }

    void reset() noexcept
    {
        delayLine.reset();
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Linear> delayLine;
    double sr { 48000.0 };
    float g { 0.7f };
};

} // namespace sendbloom
