#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** LookAndFeel that leaves rotary/toggle chrome invisible so faceplate art shows through. */
class TransparentControlsLookAndFeel : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override
    {
    }

    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override
    {
    }

    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override
    {
    }

    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override
    {
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox&) override
    {
        // Keep a faint hit-target outline only while open/focused is unnecessary; faceplate has the chrome.
        juce::ignoreUnused (g, width, height);
    }
};

} // namespace sendbloom::ui
