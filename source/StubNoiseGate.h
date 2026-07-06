#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace sendbloom
{

class StubNoiseGate
{
public:
    void prepare (double sampleRate, float releaseMs, float floorGain) noexcept
    {
        releaseCoeff = std::exp (-1.0f / (releaseMs * 0.001f * static_cast<float> (sampleRate)));
        attackCoeff = std::exp (-1.0f / (2.0f * 0.001f * static_cast<float> (sampleRate)));
        softFloor = floorGain;
        gain = 1.0f;
    }

    float process (float inputEnvelope, float thresholdDb) noexcept
    {
        const auto thresh = juce::Decibels::decibelsToGain (thresholdDb);

        if (softFloor <= 0.0f && inputEnvelope <= thresh)
        {
            gain = 0.0f;
            return gain;
        }

        const auto target = inputEnvelope > thresh ? 1.0f : softFloor;
        const auto coeff = target > gain ? attackCoeff : releaseCoeff;
        gain = coeff * gain + (1.0f - coeff) * target;
        return gain;
    }

    void reset() noexcept { gain = 1.0f; }

private:
    float gain { 1.0f };
    float softFloor { 0.0f };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
};

} // namespace sendbloom
