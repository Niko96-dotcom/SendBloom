#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace sendbloom::ui
{

class PedalKnob : public juce::Component
{
public:
    PedalKnob (juce::String labelText)
        : labelName (std::move (labelText))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f,
                                    true);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void paint (juce::Graphics& g) override
    {
        const auto bounds = getLocalBounds().toFloat();
        const auto labelArea = bounds.withTrimmedLeft (82.0f).withTrimmedTop (20.0f).withHeight (34.0f);

        g.setColour (labelColour);
        g.setFont (juce::FontOptions (19.0f, juce::Font::bold));
        g.drawFittedText (labelName, labelArea.toNearestInt(), juce::Justification::centredLeft, 2, 0.58f);

        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        if (minText.isNotEmpty())
            g.drawText (minText, 2, 58, 26, 14, juce::Justification::centred);
        if (maxText.isNotEmpty())
            g.drawText (maxText, 50, 58, 26, 14, juce::Justification::centred);

        const auto valueBounds = juce::Rectangle<float> (18.0f, 70.0f, 56.0f, 18.0f);
        g.setColour (juce::Colour (0xff1f2224));
        g.fillRoundedRectangle (valueBounds, 3.0f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (getDisplayValue(), valueBounds.toNearestInt(), juce::Justification::centred, false);
    }

    void resized() override
    {
        slider.setBounds (0, 0, 78, 78);
    }

    void setLabelColour (juce::Colour colour)
    {
        labelColour = colour;
        repaint();
    }

    void setRangeText (juce::String minValue, juce::String maxValue)
    {
        minText = std::move (minValue);
        maxText = std::move (maxValue);
        repaint();
    }

    void setValueFormatter (std::function<juce::String (double)> formatter)
    {
        valueFormatter = std::move (formatter);
        repaint();
    }

private:
    juce::String getDisplayValue() const
    {
        if (valueFormatter != nullptr)
            return valueFormatter (slider.getValue());

        return juce::String (slider.getValue(), 2);
    }

    juce::String labelName;
    juce::String minText;
    juce::String maxText;
    juce::Colour labelColour { juce::Colours::black };
    std::function<juce::String (double)> valueFormatter;
    juce::Slider slider;
};

} // namespace sendbloom::ui
