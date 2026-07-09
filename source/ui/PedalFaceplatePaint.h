#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Draw faceplate image + state overlays, or procedural chassis fallback. */
void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour cyan,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded);

} // namespace sendbloom::ui
