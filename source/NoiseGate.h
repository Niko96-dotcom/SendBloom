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
        sampleRate_ = sampleRate;

        switch (profile)
        {
            case GateProfile::PreSoft:
                releaseMs = 150.0f;
                attackMs = 2.0f;
                floorGain = juce::Decibels::decibelsToGain (-80.0f);
                postHardClose = false;
                break;

            case GateProfile::PostHard:
                // ADR-V1-11: 0.75 ms deterministic close ramp; 0.5 ms attack.
                releaseMs = 0.75f;
                attackMs = 0.5f;
                floorGain = 0.0f;
                postHardClose = true;
                break;
        }

        releaseCoeff = coeffForMs (releaseMs, sampleRate);
        attackCoeff = coeffForMs (attackMs, sampleRate);
        closeRampStep = 0.0f;
        if (postHardClose && sampleRate > 0.0)
        {
            const auto samples = std::max (1.0, releaseMs * 0.001 * sampleRate);
            closeRampStep = static_cast<float> (1.0 / samples);
        }

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

        if (postHardClose)
        {
            if (isOpen)
            {
                // 0.5 ms attack toward open.
                gain = attackCoeff * gain + (1.0f - attackCoeff) * 1.0f;
            }
            else
            {
                // Deterministic linear ramp to zero over 0.75 ms (no one-sample snap).
                gain = std::max (0.0f, gain - closeRampStep);
            }

            return gain;
        }

        const auto target = isOpen ? 1.0f : floorGain;
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
    float closeRampStep { 0.0f };
    double sampleRate_ { 48000.0 };
    bool isOpen { true };
    bool postHardClose { false };
};

} // namespace sendbloom
