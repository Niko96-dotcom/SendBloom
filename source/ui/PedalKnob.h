#pragma once
#include "NikoClearLookAndFeel.h"
#include <BinaryData.h>
#include <functional>
namespace sendbloom::ui
{
/** Shared brand rotary with a permanent legend and live value carrier. */
class PedalKnob : public juce::Component
{
public:
    PedalKnob (juce::String labelText, const void* = nullptr, size_t = 0)
        : labelName (std::move (labelText))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.25f,
                                   juce::MathConstants<float>::pi * 2.75f, true);
        slider.setMouseCursor (juce::MouseCursor::UpDownResizeCursor);
        slider.setMouseDragSensitivity (240);
        slider.setVelocityModeParameters (0.18, 1, 0.0, true, juce::ModifierKeys::shiftModifier);
        slider.setScrollWheelEnabled (true);
        slider.setWantsKeyboardFocus (true);
        slider.setName (labelName);
        slider.setTitle (labelName);
        slider.setDescription (labelName + " rotary control");
        slider.setHelpText ("Use the arrow keys to adjust; hold Shift for fine control. "
                            "Option-click or double-click resets the value.");
        slider.setTooltip (labelName + ": drag vertically, Shift-drag for fine control, "
                           "or Option-click/double-click to reset");
        slider.onValueChange = [this] { repaint(); if (sceneMode && getParentComponent() != nullptr) getParentComponent()->repaint(); };
        addAndMakeVisible (slider);
    }
    juce::Slider& getSlider() noexcept { return slider; }
    void setSceneGeometry (juce::Rectangle<int> input, juce::Rectangle<float> value)
    {
        sceneMode = true;
        sceneInput = input;
        sceneValue = value;
        resized();
    }
    void resized() override
    {
        if (sceneMode) { slider.setBounds (sceneInput); return; }
        const auto side = juce::jmin (getWidth(), getHeight() - 56);
        slider.setBounds ((getWidth() - side) / 2, 24, side, side);
    }
    void paint (juce::Graphics& g) override
    {
        if (sceneMode)
        {
            g.setColour (juce::Colour(0xffeee9db));
            g.setFont (niko::clear::numeric (13.0f));
            g.drawText (getDisplayValue(), sceneValue, juce::Justification::centred);
            if (slider.hasKeyboardFocus (true))
            {
                g.setColour (niko::clear::palette::signal);
                g.drawRoundedRectangle (sceneInput.toFloat().reduced(2.0f), 5.0f, 1.2f);
            }
            return;
        }
        const auto legend = getLocalBounds().removeFromTop (20).toFloat();
        g.setColour (niko::clear::palette::warmWhite.withAlpha (0.94f));
        if (labelBackgroundVisible) g.fillRect (legend.reduced (1.0f, 0.0f));
        g.setColour (labelColour);
        g.setFont (niko::clear::sans (12.0f, true));
        g.drawText (labelName.toUpperCase(), legend, juce::Justification::centred);
        auto value = getLocalBounds().withTop (getHeight() - 28).toFloat();
        if (auto* shared = dynamic_cast<niko::clear::LookAndFeel*> (&getLookAndFeel()))
            value = shared->paintValueWindow (g, value, isEnabled(), slider.hasKeyboardFocus (true));
        else
        {
            g.setColour (juce::Colour (0xff232a2b));
            g.fillRect (value);
        }
        g.setColour (juce::Colour (0xfff0ead6));
        g.setFont (niko::clear::numeric (14.0f));
        g.drawText (getDisplayValue(), value, juce::Justification::centred);
    }

    void setLabelBackgroundVisible (bool visible) { labelBackgroundVisible = visible; repaint(); }
    void setLabelColour (juce::Colour colour) { labelColour = colour; }
    void setDefaultValue (double value) { slider.setDoubleClickReturnValue (true, value); }
    void setValueFormatter (std::function<juce::String (double)> formatter)
    { valueFormatter = std::move (formatter); repaint(); }
    juce::String getDisplayValue() const
    { return valueFormatter ? valueFormatter (slider.getValue()) : juce::String (slider.getValue(), 2); }
private:
    bool sceneMode = false;
    bool labelBackgroundVisible = true;
    juce::Rectangle<int> sceneInput;
    juce::Rectangle<float> sceneValue;
    juce::String labelName;
    std::function<juce::String (double)> valueFormatter;
    juce::Colour labelColour { 0xff292d2e };
    juce::Slider slider;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalKnob)
};
}
