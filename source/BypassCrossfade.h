#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace sendbloom
{

struct BypassCrossfade
{
    static void processBlock (juce::AudioBuffer<float>& buffer,
                              const juce::AudioBuffer<float>& dryCopy,
                              juce::SmoothedValue<float>& wetMix) noexcept
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto wet = wetMix.getNextValue();
            const auto dry = 1.0f - wet;

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto drySample = dryCopy.getReadPointer (channel)[sample];
                auto& wetSample = buffer.getWritePointer (channel)[sample];
                wetSample = drySample * dry + wetSample * wet;
            }
        }
    }
};

} // namespace sendbloom
