#pragma once

#include <cmath>
#include <juce_dsp/juce_dsp.h>

namespace sendbloom
{

class DampedComb
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

        dampingFilter.prepare (spec);
        dampingFilter.reset();
        dampingFilter.setType (juce::dsp::StateVariableTPTFilterType::lowpass);
    }

    void setDelay (float delaySamples) noexcept
    {
        delaySamplesStored = delaySamples;
        delayLine.setDelay (delaySamples);
    }

    void setDampingCutoff (float hz) noexcept
    {
        dampingFilter.setCutoffFrequency (juce::jlimit (100.0f, static_cast<float> (sr * 0.45), hz));
    }

    void setFeedbackForRT60 (float rt60Seconds, float referenceDelaySamples) noexcept
    {
        const auto rt60 = juce::jmax (rt60Seconds, 0.05f);
        const auto delaySec = referenceDelaySamples / static_cast<float> (sr);
        feedback = std::exp (-6.9078f * delaySec / rt60);
        feedback = juce::jlimit (0.0f, 0.98f, feedback);
    }

    float processSample (float input) noexcept
    {
        const auto delayed = delayLine.popSample (0);
        const auto filtered = dampingFilter.processSample (0, delayed);
        delayLine.pushSample (0, input + filtered * feedback);
        return delayed;
    }

    void reset() noexcept
    {
        delayLine.reset();
        dampingFilter.reset();
    }

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> delayLine;
    juce::dsp::StateVariableTPTFilter<float> dampingFilter;
    double sr { 48000.0 };
    float delaySamplesStored { 1000.0f };
    float feedback { 0.8f };
};

} // namespace sendbloom
