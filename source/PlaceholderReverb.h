#pragma once

#include <juce_dsp/juce_dsp.h>
#include <cmath>

namespace sendbloom
{

class PlaceholderReverb
{
public:
    void prepare (double sampleRate, int /*maxBlockSize*/) noexcept
    {
        sr = sampleRate;
        juce::dsp::ProcessSpec spec;
        spec.sampleRate = sampleRate;
        spec.maximumBlockSize = 512;
        spec.numChannels = 1;

        delayLine.setMaximumDelayInSamples (48000);
        delayLine.prepare (spec);
        delayLine.reset();

        delaySamples = static_cast<float> (sampleRate * 0.3);
        delayLine.setDelay (delaySamples);
    }

    float processSample (float input, float rt60Seconds) noexcept
    {
        const auto delayed = delayLine.popSample (0);
        const auto feedback = rt60ToFeedback (rt60Seconds);
        delayLine.pushSample (0, input + delayed * feedback);
        return juce::jlimit (-4.0f, 4.0f, delayed);
    }

private:
    float rt60ToFeedback (float rt60Seconds) const noexcept
    {
        const auto rt60 = juce::jmax (rt60Seconds, 0.05f);
        const auto delaySec = delaySamples / static_cast<float> (sr);
        const auto feedback = std::exp (-6.9078f * delaySec / rt60);
        return juce::jlimit (0.0f, 0.95f, feedback);
    }

    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> delayLine;
    double sr { 48000.0 };
    float delaySamples { 14400.0f };
};

} // namespace sendbloom
