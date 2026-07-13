#pragma once

#include "TransparentControls.h"

#include <BinaryData.h>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Photographed rotary control: a square knob crop that rotates with the value,
    plus a caption strip underneath. The strip shows the control's name and swaps
    to the live value while the knob is hovered or dragged. */
class PedalKnob : public juce::Component
{
public:
    PedalKnob (juce::String labelText, const void* imageData = nullptr, size_t imageSize = 0)
        : labelName (std::move (labelText))
    {
        slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
        slider.setRotaryParameters (juce::MathConstants<float>::pi * 1.2f,
                                    juce::MathConstants<float>::pi * 2.8f,
                                    true);
        slider.setLookAndFeel (&transparentLnf);
        slider.setOpaque (false);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);

        knobImage = loadKnobImage (imageData, imageSize);
    }

    ~PedalKnob() override
    {
        slider.setLookAndFeel (nullptr);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void paintOverChildren (juce::Graphics& g) override
    {
        paintKnob (g);
        paintShading (g);
        paintCaption (g);
    }

    void resized() override
    {
        const auto size = knobSize();
        slider.setBounds ((getWidth() - size) / 2, 0, size, size);
    }

    void setLabelColour (juce::Colour colour)
    {
        labelColour = colour;
        valueColour = colour;
        engravedCaption = false; // custom colour means a dark panel, not the plate
    }

    void setRangeText (juce::String, juce::String) {}

    void setValueFormatter (std::function<juce::String (double)> formatter)
    {
        valueFormatter = std::move (formatter);
        repaint();
    }

    juce::String getDisplayValue() const
    {
        if (valueFormatter != nullptr)
            return valueFormatter (slider.getValue());

        return juce::String (slider.getValue(), 2);
    }

private:
    static juce::Image loadKnobImage (const void* imageData, size_t imageSize)
    {
        auto image = imageData != nullptr && imageSize > 0
                   ? juce::ImageFileFormat::loadFrom (imageData, imageSize)
                   : juce::Image();
        if (! image.isValid())
            image = juce::ImageFileFormat::loadFrom (
                juce::File::getCurrentWorkingDirectory().getChildFile ("resources/ui/knob.png"));
        if (! image.isValid())
            image = juce::ImageFileFormat::loadFrom (BinaryData::knob_png,
                                                     static_cast<size_t> (BinaryData::knob_pngSize));
        return image;
    }

    // Knob square is as wide as the component; whatever height remains is the caption strip.
    int knobSize() const noexcept { return juce::jmin (getWidth(), getHeight()); }

    void paintKnob (juce::Graphics& g)
    {
        if (! knobImage.isValid())
            return;

        const auto bounds = slider.getBounds().toFloat();
        juce::Graphics::ScopedSaveState clip (g);
        g.reduceClipRegion (slider.getBounds());

        const auto params = slider.getRotaryParameters();
        const auto t = static_cast<float> (slider.valueToProportionOfLength (slider.getValue()));
        const auto angle = params.startAngleRadians
                         + t * (params.endAngleRadians - params.startAngleRadians);

        g.addTransform (juce::AffineTransform::rotation (angle, bounds.getCentreX(), bounds.getCentreY()));
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (knobImage, bounds, juce::RectanglePlacement::centred, false);
    }

    // Relight the rotated photo: the key light must stay fixed above the pedal, so
    // the cap reads as a curved object instead of a flat spinning disc.
    void paintShading (juce::Graphics& g)
    {
        const auto circle = slider.getBounds().toFloat().reduced (
            slider.getBounds().toFloat().getWidth() * 0.06f);

        juce::ColourGradient shade (juce::Colours::white.withAlpha (0.15f),
                                    circle.getCentreX(), circle.getY(),
                                    juce::Colours::black.withAlpha (0.22f),
                                    circle.getCentreX(), circle.getBottom(), false);
        shade.addColour (0.45, juce::Colours::transparentBlack);
        g.setGradientFill (shade);
        g.fillEllipse (circle);
    }

    void paintCaption (juce::Graphics& g)
    {
        const auto strip = getLocalBounds().withTop (knobSize());
        if (strip.isEmpty())
            return;

        const auto showValue = slider.isMouseOverOrDragging();
        const auto text = showValue ? getDisplayValue() : labelName.toUpperCase();
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        if (engravedCaption)
        {
            g.setColour (juce::Colours::white.withAlpha (0.30f));
            g.drawText (text, strip.translated (0, 1), juce::Justification::centred, false);
        }
        g.setColour ((showValue ? valueColour : labelColour).withAlpha (0.94f));
        g.drawText (text, strip, juce::Justification::centred, false);
    }

    juce::String labelName;
    std::function<juce::String (double)> valueFormatter;
    juce::Colour labelColour { 0xff161413 };
    juce::Colour valueColour { 0xffe66c0b };
    bool engravedCaption { true };
    TransparentControlsLookAndFeel transparentLnf;
    juce::Slider slider;
    juce::Image knobImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalKnob)
};

} // namespace sendbloom::ui
