#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

class PedalKnob : public juce::Component
{
public:
    PedalKnob (juce::String labelText)
    {
        label.setText (labelText, juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);

        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        addAndMakeVisible (slider);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void resized() override
    {
        auto area = getLocalBounds();
        label.setBounds (area.removeFromBottom (16));
        slider.setBounds (area);
    }

    void setLabelColour (juce::Colour colour)
    {
        label.setColour (juce::Label::textColourId, colour);
    }

private:
    juce::Label label;
    juce::Slider slider;
};

} // namespace sendbloom::ui
