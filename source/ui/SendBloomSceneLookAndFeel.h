#pragma once
#include "NikoClearLookAndFeel.h"
namespace sendbloom::ui
{
class SendBloomSceneLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawRotarySlider (juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override {}
    void drawToggleButton (juce::Graphics&, juce::ToggleButton&, bool, bool) override {}
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override {}
    void drawButtonText (juce::Graphics&, juce::TextButton&, bool, bool) override {}
    void drawComboBox (juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override {}
    juce::Font getComboBoxFont (juce::ComboBox&) override { return niko::clear::numeric(13.0f); }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        const auto& props = box.getProperties();
        label.setBounds (juce::Rectangle<float>(float(props["scene.text.x"]), float(props["scene.text.y"]), float(props["scene.text.width"]), float(props["scene.text.height"])).getLargestIntegerWithin());
        label.setFont (getComboBoxFont(box));
        label.setJustificationType (juce::Justification::centred);
        label.setBorderSize (juce::BorderSize<int>(0));
        label.setColour (juce::Label::textColourId, juce::Colour(0xffeee9db));
    }
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        if (! label.isBeingEdited())
        {
            g.setColour (juce::Colour(0xffeee9db));
            g.setFont (label.getFont());
            g.drawFittedText (label.getText(), label.getLocalBounds(), juce::Justification::centred, 1);
        }
    }
};
}
