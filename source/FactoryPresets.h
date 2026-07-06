#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <array>

namespace sendbloom
{

class FactoryPresets
{
public:
    static constexpr int kNumPresets = 8;

    static juce::String getPresetName (int index);
    static juce::ValueTree makePresetState (int index);
    static void applyPreset (juce::AudioProcessorValueTreeState& apvts, int index);
};

} // namespace sendbloom
