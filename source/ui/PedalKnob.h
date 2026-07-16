#pragma once

#include "PedalFaceplatePaint.h"
#include "TransparentControls.h"

#include <BinaryData.h>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

namespace sendbloom::ui
{

/** Path-traced rotary control: a vertical filmstrip (one frame per pointer
    angle, rendered by tools/render_ui.py in the faceplate's light rig) plus a
    caption strip underneath. The strip shows the control's name and swaps to
    the live value while the knob is hovered or dragged.

    The knob never rotates an image. Each frame was lit for its own pointer
    angle, so the room's reflection stays put while the pointer moves — which
    is why this class has no hand-painted shading pass any more. */
class PedalKnob : public juce::Component
{
public:
    PedalKnob (juce::String labelText, const void* stripData = nullptr, size_t stripSize = 0)
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

        strip = loadStrip (stripData, stripSize);
        stripLo = boxHalveImage (strip);
    }

    ~PedalKnob() override
    {
        slider.setLookAndFeel (nullptr);
    }

    juce::Slider& getSlider() noexcept { return slider; }

    void paintOverChildren (juce::Graphics& g) override
    {
        paintKnob (g);
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
    static juce::Image loadStrip (const void* data, size_t size)
    {
        auto image = juce::ImageFileFormat::loadFrom (data, size);
        if (! image.isValid())
            image = juce::ImageFileFormat::loadFrom (
                BinaryData::knob_large_strip_png,
                static_cast<size_t> (BinaryData::knob_large_strip_pngSize));
        return image;
    }

    // Knob square is as wide as the component; whatever height remains is the caption strip.
    int knobSize() const noexcept { return juce::jmin (getWidth(), getHeight()); }

    int frameCount() const noexcept
    {
        return strip.isValid() ? juce::jmax (1, strip.getHeight() / strip.getWidth()) : 0;
    }

    void paintKnob (juce::Graphics& g)
    {
        const auto frames = frameCount();
        if (frames == 0)
            return;

        const auto t = slider.valueToProportionOfLength (slider.getValue());
        const auto frame = juce::roundToInt (juce::jlimit (0.0, 1.0, t) * (frames - 1));

        // Standard-DPI contexts get the box-halved strip drawn 1:1; JUCE's own
        // 2:1 resampling would blur away the cap's machined micro-texture.
        const auto& art = wantsHiResArt (g) ? strip : stripLo;
        const auto side = art.getWidth();

        const auto bounds = slider.getBounds();
        g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
        g.drawImage (art,
                     bounds.getX(), bounds.getY(), bounds.getWidth(), bounds.getHeight(),
                     0, frame * side, side, side);
    }

    void paintCaption (juce::Graphics& g)
    {
        const auto strip_ = getLocalBounds().withTop (knobSize());
        if (strip_.isEmpty())
            return;

        const auto showValue = slider.isMouseOverOrDragging();
        const auto text = showValue ? getDisplayValue() : labelName.toUpperCase();
        g.setFont (juce::FontOptions (11.0f, juce::Font::bold));
        if (engravedCaption)
        {
            g.setColour (juce::Colours::white.withAlpha (0.30f));
            g.drawText (text, strip_.translated (0, 1), juce::Justification::centred, false);
        }
        g.setColour ((showValue ? valueColour : labelColour).withAlpha (0.94f));
        g.drawText (text, strip_, juce::Justification::centred, false);
    }

    juce::String labelName;
    std::function<juce::String (double)> valueFormatter;
    juce::Colour labelColour { 0xff161413 };
    juce::Colour valueColour { 0xffe66c0b };
    bool engravedCaption { true };
    TransparentControlsLookAndFeel transparentLnf;
    juce::Slider slider;
    juce::Image strip;
    juce::Image stripLo;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PedalKnob)
};

} // namespace sendbloom::ui
