#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

class ClipLed : public juce::Component, private juce::Timer
{
public:
    explicit ClipLed (std::function<bool()> isActiveFn)
        : getIsActive (std::move (isActiveFn))
    {
        label.setText ("CLIP", juce::dontSendNotification);
        label.setJustificationType (juce::Justification::centred);
        label.setInterceptsMouseClicks (false, false);
        addAndMakeVisible (label);
        startTimerHz (30);
    }

    void paint (juce::Graphics& g) override
    {
        const auto ledBounds = getLocalBounds().removeFromTop (getHeight() - 14).toFloat().reduced (4.0f);
        g.setColour (active ? juce::Colour (0xffFF3B30) : juce::Colour (0xff4A4A50));
        g.fillEllipse (ledBounds);
    }

    void resized() override
    {
        label.setBounds (getLocalBounds().removeFromBottom (12));
        label.setColour (juce::Label::textColourId, juce::Colour (0xffE8E6E3));
        label.setFont (juce::FontOptions (9.0f));
    }

private:
    void timerCallback() override
    {
        const auto nowActive = getIsActive != nullptr && getIsActive();
        if (nowActive != active)
        {
            active = nowActive;
            repaint();
        }
    }

    std::function<bool()> getIsActive;
    bool active { false };
    juce::Label label;
};

} // namespace sendbloom::ui
