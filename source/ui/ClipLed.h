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
        auto bounds = getLocalBounds().toFloat();
        const auto lamp = bounds.removeFromTop (34.0f).reduced (5.0f);

        g.setColour (juce::Colours::black.withAlpha (0.75f));
        g.fillEllipse (lamp.expanded (3.0f));

        juce::ColourGradient lampFill (active ? juce::Colour (0xffff6b5c) : juce::Colour (0xff7b2f2c),
                                       lamp.getX(), lamp.getY(),
                                       active ? juce::Colour (0xffd31316) : juce::Colour (0xff281d1b),
                                       lamp.getRight(), lamp.getBottom(), false);
        g.setGradientFill (lampFill);
        g.fillEllipse (lamp);

        g.setColour (juce::Colours::white.withAlpha (active ? 0.42f : 0.12f));
        g.fillEllipse (lamp.reduced (8.0f).translated (-3.0f, -4.0f));

        const auto meter = juce::Rectangle<float> (6.0f, 62.0f, 24.0f, 116.0f);
        juce::ColourGradient meterFill (juce::Colour (0xff273426), meter.getX(), meter.getBottom(),
                                        juce::Colour (0xff59644a), meter.getX(), meter.getY(), false);
        g.setGradientFill (meterFill);
        g.fillRoundedRectangle (meter, 4.0f);
        g.setColour (juce::Colours::black);
        g.drawRoundedRectangle (meter, 4.0f, 2.0f);

        g.setColour (active ? juce::Colour (0xffff1212) : juce::Colour (0xffcc1517));
        g.fillRect (meter.reduced (8.0f, 8.0f).removeFromTop (19.0f));
        g.setColour (juce::Colour (0xffff6f61));
        g.fillRect (meter.reduced (8.0f, 38.0f).removeFromTop (11.0f));

        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (15.0f, juce::Font::bold));
        g.drawText ("CLIP", 0, 40, getWidth(), 20, juce::Justification::centred);
        g.drawText ("OVLD", 0, getHeight() - 20, getWidth(), 20, juce::Justification::centred);
    }

    void resized() override
    {
        label.setVisible (false);
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
