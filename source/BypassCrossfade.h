#pragma once

#include <juce_audio_basics/juce_audio_basics.h>

namespace sendbloom
{

struct BypassCrossfade
{
    // ADR-V1-10: final = originalDry * (1 - engagedMix) + processed * engagedMix.
    // engagedMix=0 → dry only; engagedMix=1 → fully engaged (output-gained) path.
    static float mixSample (float originalDry, float processed, float engagedMix) noexcept
    {
        return originalDry * (1.0f - engagedMix) + processed * engagedMix;
    }

    static void processBlock (juce::AudioBuffer<float>& buffer,
                              const juce::AudioBuffer<float>& dryCopy,
                              juce::SmoothedValue<float>& wetMix) noexcept
    {
        const auto numChannels = buffer.getNumChannels();
        const auto numSamples = buffer.getNumSamples();

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto engagedMix = wetMix.getNextValue();

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto drySample = dryCopy.getReadPointer (channel)[sample];
                auto& wetSample = buffer.getWritePointer (channel)[sample];
                wetSample = mixSample (drySample, wetSample, engagedMix);
            }
        }
    }
};

} // namespace sendbloom
