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

    void drawButtonBackground (juce::Graphics& g, juce::Button& button,
                               const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted,
                               bool shouldDrawButtonAsDown) override;

    void drawButtonText (juce::Graphics& g, juce::TextButton& button,
                         bool shouldDrawButtonAsHighlighted,
                         bool shouldDrawButtonAsDown) override;

    void drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox& box) override;

    juce::Font getComboBoxFont (juce::ComboBox& box) override;

    juce::Colour chassisColour() const noexcept { return chassis; }
    juce::Colour facePlateColour() const noexcept { return facePlate; }
    juce::Colour accentColour() const noexcept { return accent; }
    juce::Colour labelColour() const noexcept { return labelText; }
    juce::Colour cyanColour() const noexcept { return cyan; }

private:
    juce::Colour chassis;
    juce::Colour facePlate;
    juce::Colour accent;
    juce::Colour labelText;
    juce::Colour cyan;
};

} // namespace sendbloom::ui
