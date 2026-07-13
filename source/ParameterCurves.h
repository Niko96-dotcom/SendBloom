#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace sendbloom::ParameterCurves
{

inline float smoothstep (float x) noexcept
{
    return x * x * (3.0f - 2.0f * x);
}

inline float sizeToRT60 (float sizeNorm) noexcept
{
    return 0.25f + 5.75f * std::pow (sizeNorm, 2.4f);
}

inline float distnBlend (float norm) noexcept
{
    return std::pow (norm, 2.8f);
}

inline void levelEqualPower (float norm, float& dry, float& wet) noexcept
{
    // ADR-V1-09: Level scales wet return only; dry stays unity.
    constexpr auto halfPi = juce::MathConstants<float>::halfPi;
    dry = 1.0f;
    wet = std::sin (halfPi * norm);
}

inline float inputGainDb (float norm) noexcept
{
    // ADR-V1-08: canonical −9 / 0 / +9 at 0 / 0.5 / 1.
    return -9.0f + 18.0f * smoothstep (norm);
}

// Fixed reference gate threshold and the +/- trim range around it.
inline constexpr float kGateReferenceDb = -45.0f;
inline constexpr float kGateTrimDb = 6.0f;

inline float inputThresholdDb (float norm) noexcept
{
    // The reference hardware has no threshold knob: the input-level control alone
    // drives the guitar into a fixed gate. Here the input gain stays the dominant
    // sensitivity control — it scales the detector envelope, so turning it up
    // effectively lowers the threshold — and this parameter is only a small
    // +/-6 dB calibration trim around a fixed reference, centred (0 dB) at norm 0.5.
    return kGateReferenceDb + (norm - 0.5f) * 2.0f * kGateTrimDb;
}

inline float outputGainLinear (float gainDb) noexcept
{
    return juce::Decibels::decibelsToGain (gainDb);
}

inline float sendGain (float amount, bool firmFeel) noexcept
{
    const auto s = smoothstep (amount);
    return firmFeel ? std::pow (s, 1.85f) : std::pow (s, 1.2f);
}

} // namespace sendbloom::ParameterCurves
