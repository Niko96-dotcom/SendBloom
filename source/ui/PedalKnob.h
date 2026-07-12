#pragma once

#include "TransparentControls.h"

#include <BinaryData.h>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Rotary hotspot: covers the faceplate knob, then paints a rotating crop of it. */
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
        slider.setLookAndFeel (&transparentLnf);
        slider.setOpaque (false);
        slider.onValueChange = [this] { repaint(); };
        addAndMakeVisible (slider);

        knobImage = loadKnobImage();
    }

    ~PedalKnob() override
    {
        slider.setLookAndFeel (nullptr);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void paint (juce::Graphics& g) override
    {
        paintKnob (g);
        paintValue (g);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        paintKnob (g);
        paintValue (g);
    }

    void resized() override
    {
        slider.setBounds (0, 0, kKnobSize, kKnobSize);
    }

    void setLabelColour (juce::Colour) {}
    void setRangeText (juce::String, juce::String) {}

    void setValueFormatter (std::function<juce::String (double)> formatter)
    {
        valueFormatter = std::move (formatter);
        repaint();
    }

    void setValueBounds (juce::Rectangle<int> boundsInComponent)
    {
        valueBounds = boundsInComponent;
        repaint();
    }

private:
    static juce::Image loadKnobImage()
    {
        auto image = juce::ImageFileFormat::loadFrom (
            juce::File::getCurrentWorkingDirectory().getChildFile ("resources/ui/knob.png"));
        if (! image.isValid())
            image = juce::ImageFileFormat::loadFrom (BinaryData::knob_png,
                                                     static_cast<size_t> (BinaryData::knob_pngSize));
        return image;
    }

    void paintKnob (juce::Graphics& g)
    {
        if (! knobImage.isValid())
            return;

        const auto centre = juce::Point<float> (static_cast<float> (kKnobSize) * 0.5f,
                                                static_cast<float> (kKnobSize) * 0.5f);
        // Slightly inside the cyan ring so soft edges sit on the faceplate gap, not over it.
        const auto coverRadius = 23.0f;

        {
            juce::Graphics::ScopedSaveState clip (g);
            g.reduceClipRegion (juce::Rectangle<int> (0, 0, kKnobSize, kKnobSize));

            // Opaque disc under the sprite hides the static faceplate pointer while dragging.
            g.setColour (juce::Colour (0xff303030));
            g.fillEllipse (centre.x - coverRadius, centre.y - coverRadius,
                           coverRadius * 2.0f, coverRadius * 2.0f);

            const auto params = slider.getRotaryParameters();
            const auto t = static_cast<float> (slider.valueToProportionOfLength (slider.getValue()));
            const auto angle = params.startAngleRadians
                             + t * (params.endAngleRadians - params.startAngleRadians);
            const auto dest = juce::Rectangle<float> (static_cast<float> (kKnobSize),
                                                      static_cast<float> (kKnobSize));

            g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));
            g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
            g.drawImage (knobImage, dest, juce::RectanglePlacement::centred, false);
        }
    }

    void paintValue (juce::Graphics& g)
    {
        g.setColour (juce::Colours::white.withAlpha (0.95f));
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (getDisplayValue(), valueBounds, juce::Justification::centred, false);
    }

    juce::String getDisplayValue() const
    {
        if (valueFormatter != nullptr)
            return valueFormatter (slider.getValue());

        return juce::String (slider.getValue(), 2);
    }

    static constexpr int kKnobSize = 50;

    juce::String labelName;
    std::function<juce::String (double)> valueFormatter;
    juce::Rectangle<int> valueBounds { 0, 60, 51, 15 };
    TransparentControlsLookAndFeel transparentLnf;
    juce::Slider slider;
    juce::Image knobImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalKnob)
};

} // namespace sendbloom::ui
