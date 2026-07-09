#pragma once

#include "TransparentControls.h"

#include <BinaryData.h>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

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

        const auto valueBounds = juce::Rectangle<int> (2, kKnobSize + 5, 52, 15);
        g.setColour (juce::Colours::white.withAlpha (0.92f));
        g.setFont (juce::FontOptions (13.0f, juce::Font::bold));
        g.drawText (getDisplayValue(), valueBounds, juce::Justification::centred, false);
    }

    void paintOverChildren (juce::Graphics& g) override
    {
        // Slider child paints after us; redraw the rotating knob on top so it stays visible.
        paintKnob (g);
    }

    void resized() override
    {
        slider.setBounds (0, 0, kKnobSize, kKnobSize);
    }

    void setLabelColour (juce::Colour)
    {
    }

    void setRangeText (juce::String, juce::String)
    {
    }

    void setValueFormatter (std::function<juce::String (double)> formatter)
    {
        valueFormatter = std::move (formatter);
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
        const auto params = slider.getRotaryParameters();
        const auto t = static_cast<float> (slider.valueToProportionOfLength (slider.getValue()));
        const auto angle = params.startAngleRadians
                         + t * (params.endAngleRadians - params.startAngleRadians);
        // Keep art inside the faceplate cyan ring (~48px body inside a 64px hit target).
        const auto dest = juce::Rectangle<float> (static_cast<float> (kKnobArtSize),
                                                  static_cast<float> (kKnobArtSize))
                              .withCentre (centre);

        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::rotation (angle, centre.x, centre.y));
        g.drawImage (knobImage, dest);
    }

    juce::String getDisplayValue() const
    {
        if (valueFormatter != nullptr)
            return valueFormatter (slider.getValue());

        return juce::String (slider.getValue(), 2);
    }

    static constexpr int kKnobSize = 64;
    static constexpr int kKnobArtSize = 48;

    juce::String labelName;
    std::function<juce::String (double)> valueFormatter;
    TransparentControlsLookAndFeel transparentLnf;
    juce::Slider slider;
    juce::Image knobImage;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalKnob)
};

} // namespace sendbloom::ui
