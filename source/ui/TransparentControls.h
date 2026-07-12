#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Invisible chrome for hotspots sitting on faceplate art. */
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

    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override
    {
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        // Match the baked faceplate preset type size (~13–14 px tall field).
        return juce::FontOptions (13.0f, juce::Font::bold);
    }

    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (8, 0, juce::jmax (1, box.getWidth() - 28), box.getHeight());
        label.setFont (getComboBoxFont (box));
        label.setJustificationType (juce::Justification::centredLeft);
    }

    juce::Label* createComboBoxTextBox (juce::ComboBox& box) override
    {
        auto* label = LookAndFeel_V4::createComboBoxTextBox (box);
        // Faceplate paints the preset name; keep the ComboBox label invisible.
        label->setColour (juce::Label::textColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::backgroundColourId, juce::Colours::transparentBlack);
        label->setColour (juce::Label::outlineColourId, juce::Colours::transparentBlack);
        label->setInterceptsMouseClicks (false, false);
        return label;
    }
};

} // namespace sendbloom::ui
