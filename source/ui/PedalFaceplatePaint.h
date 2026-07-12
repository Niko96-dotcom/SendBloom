#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** True when footswitch pressed overlay should draw (press/amount — not connection alone). */
bool shouldDrawFootswitchPressedOverlay (bool padPressed, float displayAmount, float sendAmountNorm) noexcept;

/** Draw faceplate image + state overlays, or procedural chassis fallback. */
void paintPedalFaceplate (juce::Graphics& g,
                          juce::Rectangle<float> bounds,
                          juce::Colour cyan,
                          juce::AudioProcessorValueTreeState& apvts,
                          bool clipActive,
                          bool advancedExpanded,
                          bool padPressed = false,
                          float padDisplayAmount = 0.0f);

} // namespace sendbloom::ui
