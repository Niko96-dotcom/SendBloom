#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

class SendBloomLookAndFeel : public juce::LookAndFeel_V4
{
public:
    SendBloomLookAndFeel();

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                           bool shouldDrawButtonAsHighlighted,
                           bool shouldDrawButtonAsDown) override;

    juce::Colour chassisColour() const noexcept { return chassis; }
    juce::Colour facePlateColour() const noexcept { return facePlate; }
    juce::Colour accentColour() const noexcept { return accent; }
    juce::Colour labelColour() const noexcept { return labelText; }

private:
    juce::Colour chassis;
    juce::Colour facePlate;
    juce::Colour accent;
    juce::Colour labelText;
};

} // namespace sendbloom::ui
