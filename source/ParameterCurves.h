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
    constexpr auto halfPi = juce::MathConstants<float>::halfPi;
    dry = std::sin (halfPi * (1.0f - norm));
    wet = std::sin (halfPi * norm);
}

inline float inputGainDb (float norm) noexcept
{
    const auto t = smoothstep (norm);
    return 9.0f + t * (-3.0f - 9.0f);
}

inline float inputThresholdDb (float norm) noexcept
{
    const auto t = std::pow (norm, 1.6f);
    return -52.0f + t * (-18.0f - (-52.0f));
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
