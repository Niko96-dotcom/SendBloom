#include "SendBloomLookAndFeel.h"

namespace sendbloom::ui
{

SendBloomLookAndFeel::SendBloomLookAndFeel()
{
    chassis = juce::Colour (0xff2A2A2E);
    facePlate = juce::Colour (0xff3D3D42);
    accent = juce::Colour (0xffE8A838);
    labelText = juce::Colour (0xffE8E6E3);

    setColour (juce::ResizableWindow::backgroundColourId, chassis);
    setColour (juce::Slider::textBoxTextColourId, labelText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, labelText);
    setColour (juce::ToggleButton::tickColourId, accent);
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff6A6A70));
    setColour (juce::ComboBox::backgroundColourId, facePlate);
    setColour (juce::ComboBox::textColourId, labelText);
    setColour (juce::ComboBox::outlineColourId, facePlate.brighter (0.15f));
    setColour (juce::PopupMenu::backgroundColourId, facePlate);
    setColour (juce::PopupMenu::textColourId, labelText);
}

void SendBloomLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPosProportional, float rotaryStartAngle,
                                             float rotaryEndAngle, juce::Slider& slider)
{
    juce::ignoreUnused (slider);

    const auto bounds = juce::Rectangle<float> (static_cast<float> (x),
                                                static_cast<float> (y),
                                                static_cast<float> (width),
                                                static_cast<float> (height))
                            .reduced (4.0f);
    const auto radius = juce::jmin (bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const auto centre = bounds.getCentre();
    const auto angle = rotaryStartAngle + sliderPosProportional * (rotaryEndAngle - rotaryStartAngle);

    g.setColour (facePlate);
    g.fillEllipse (bounds);

    g.setColour (facePlate.brighter (0.2f));
    g.drawEllipse (bounds, 1.5f);

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (accent);
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    const auto pointerLength = radius * 0.55f;
    const auto pointerThickness = 2.5f;
    juce::Path pointer;
    pointer.addRectangle (-pointerThickness * 0.5f, -radius + 6.0f,
                          pointerThickness, pointerLength);
    g.setColour (labelText);
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void SendBloomLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);
    const auto tickBounds = bounds.removeFromLeft (bounds.getHeight()).reduced (3.0f);

    g.setColour (facePlate);
    g.fillRoundedRectangle (tickBounds, 4.0f);

    if (button.getToggleState())
    {
        g.setColour (accent);
        g.fillRoundedRectangle (tickBounds.reduced (3.0f), 3.0f);
    }

    g.setColour (labelText);
    g.setFont (juce::FontOptions (11.0f));
    g.drawText (button.getButtonText(), bounds, juce::Justification::centredLeft, true);
}

} // namespace sendbloom::ui
