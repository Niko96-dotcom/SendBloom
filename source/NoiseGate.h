#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace sendbloom
{

enum class GateProfile
{
    PreSoft,
    PostHard
};

class NoiseGate
{
public:
    void prepare (double sampleRate, GateProfile profile) noexcept
    {
        switch (profile)
        {
            case GateProfile::PreSoft:
                releaseMs = 150.0f;
                attackMs = 2.0f;
                floorGain = juce::Decibels::decibelsToGain (-80.0f);
                break;

            case GateProfile::PostHard:
                releaseMs = 7.0f;
                attackMs = 0.5f;
                floorGain = 0.0f;
                break;
        }

        releaseCoeff = coeffForMs (releaseMs, sampleRate);
        attackCoeff = coeffForMs (attackMs, sampleRate);
        gain = 1.0f;
        isOpen = true;
    }

    float process (float inputEnvelope, float thresholdDb) noexcept
    {
        const auto openThresh = juce::Decibels::decibelsToGain (thresholdDb);
        const auto closeThresh = juce::Decibels::decibelsToGain (thresholdDb - 3.0f);

        if (! isOpen && inputEnvelope > openThresh)
            isOpen = true;

        if (isOpen && inputEnvelope < closeThresh)
            isOpen = false;

        const auto target = isOpen ? 1.0f : floorGain;

        if (floorGain <= 0.0f && ! isOpen)
        {
            gain = 0.0f;
            return gain;
        }

        const auto coeff = target > gain ? attackCoeff : releaseCoeff;
        gain = coeff * gain + (1.0f - coeff) * target;
        return gain;
    }

    bool getIsOpen() const noexcept { return isOpen; }
    float getGain() const noexcept { return gain; }

    void reset() noexcept
    {
        gain = 1.0f;
        isOpen = true;
    }

private:
    static float coeffForMs (float ms, double sampleRate) noexcept
    {
        return std::exp (-1.0f / (ms * 0.001f * static_cast<float> (sampleRate)));
    }

    float gain { 1.0f };
    float floorGain { 0.0f };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
    float attackMs { 2.0f };
    float releaseMs { 150.0f };
    bool isOpen { true };
};

} // namespace sendbloom
