#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <cmath>

namespace sendbloom
{

struct EngineCrossfade
{
    static constexpr float kDefaultFadeSeconds = 0.035f;
    static constexpr float kMinFadeSeconds = 0.02f;
    static constexpr float kMaxFadeSeconds = 0.05f;

    void prepare (double sampleRate) noexcept
    {
        const auto fadeSeconds = juce::jlimit (kMinFadeSeconds, kMaxFadeSeconds, kDefaultFadeSeconds);
        mixGain.reset (sampleRate, static_cast<double> (fadeSeconds));
        mixGain.setCurrentAndTargetValue (0.0f);
        crossfading = false;
    }

    void beginCrossfadeTowardFixed() noexcept
    {
        crossfading = true;
        targetIsFixed = true;
        mixGain.setTargetValue (1.0f);
    }

    void beginCrossfadeTowardHost() noexcept
    {
        crossfading = true;
        targetIsFixed = false;
        mixGain.setTargetValue (0.0f);
    }

    void mixWetBlock (const float* hostWet,
                      const float* fixedWet,
                      float* output,
                      int numSamples) noexcept
    {
        static constexpr auto halfPi = juce::MathConstants<float>::halfPi;

        for (int i = 0; i < numSamples; ++i)
        {
            const auto g = mixGain.getNextValue();
            const auto hostGain = std::cos (g * halfPi);
            const auto fixedGain = std::sin (g * halfPi);
            output[i] = hostWet[i] * hostGain + fixedWet[i] * fixedGain;
        }

        if (! mixGain.isSmoothing())
            crossfading = false;
    }

    bool isCrossfading() const noexcept
    {
        return crossfading || mixGain.isSmoothing();
    }

    bool targetIsFixedEngine() const noexcept
    {
        return targetIsFixed;
    }

private:
    juce::SmoothedValue<float> mixGain;
    bool crossfading { false };
    bool targetIsFixed { false };
};

} // namespace sendbloom
