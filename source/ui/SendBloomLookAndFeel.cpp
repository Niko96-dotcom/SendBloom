#include "SendBloomLookAndFeel.h"

namespace sendbloom::ui
{

SendBloomLookAndFeel::SendBloomLookAndFeel()
{
    chassis = juce::Colour (0xfff7f7f3);
    facePlate = juce::Colour (0xffffffff);
    accent = juce::Colour (0xffff8f25);
    labelText = juce::Colour (0xff050505);
    cyan = juce::Colour (0xffe66c0b);

    setColour (juce::ResizableWindow::backgroundColourId, chassis);
    setColour (juce::Slider::textBoxTextColourId, labelText);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::ToggleButton::textColourId, labelText);
    setColour (juce::ToggleButton::tickColourId, accent);
    setColour (juce::ToggleButton::tickDisabledColourId, juce::Colour (0xff444444));
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xffefefef));
    setColour (juce::ComboBox::textColourId, labelText);
    setColour (juce::ComboBox::outlineColourId, juce::Colours::black);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colours::white);
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

    g.setColour (cyan);
    g.drawEllipse (bounds.expanded (4.0f), 3.0f);

    juce::ColourGradient outer (juce::Colour (0xff5b5b5b), bounds.getX(), bounds.getY(),
                                juce::Colour (0xff0e0e0e), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (outer);
    g.fillEllipse (bounds);

    g.setColour (juce::Colour (0xff070707));
    g.drawEllipse (bounds.reduced (2.0f), 5.0f);

    juce::ColourGradient cap (juce::Colour (0xff333333), bounds.getX(), bounds.getY(),
                              juce::Colour (0xff101010), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (cap);
    g.fillEllipse (bounds.reduced (9.0f));

    juce::Path arc;
    arc.addCentredArc (centre.x, centre.y, radius, radius, 0.0f,
                       rotaryStartAngle, angle, true);
    g.setColour (cyan.withAlpha (0.82f));
    g.strokePath (arc, juce::PathStrokeType (3.0f, juce::PathStrokeType::curved,
                                              juce::PathStrokeType::rounded));

    const auto pointerLength = radius * 0.55f;
    const auto pointerThickness = 2.5f;
    juce::Path pointer;
    pointer.addRectangle (-pointerThickness * 0.5f, -radius + 6.0f,
                          pointerThickness, pointerLength);
    g.setColour (juce::Colour (0xffd7c4b0));
    g.fillPath (pointer, juce::AffineTransform::rotation (angle).translated (centre.x, centre.y));
}

void SendBloomLookAndFeel::drawToggleButton (juce::Graphics& g, juce::ToggleButton& button,
                                             bool shouldDrawButtonAsHighlighted,
                                             bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    const auto text = button.getButtonText();
    auto bounds = button.getLocalBounds().toFloat().reduced (2.0f);

    if (text.startsWithIgnoreCase ("Dark"))
    {
        const auto down = button.getToggleState() || shouldDrawButtonAsDown;
        juce::ColourGradient body (down ? juce::Colour (0xff171b21) : juce::Colour (0xff59616d),
                                   bounds.getX(), bounds.getY(),
                                   down ? juce::Colour (0xff59616d) : juce::Colour (0xff1f242b),
                                   bounds.getRight(), bounds.getBottom(), false);
        g.setGradientFill (body);
        g.fillRoundedRectangle (bounds, 7.0f);
        g.setColour (juce::Colour (0xff111111));
        g.drawRoundedRectangle (bounds, 7.0f, 4.0f);
        g.setColour (juce::Colours::white.withAlpha (down ? 0.08f : 0.18f));
        g.drawRoundedRectangle (bounds.reduced (5.0f), 4.0f, 1.5f);
        return;
    }

    const auto isPost = button.getToggleState();
    g.setColour (juce::Colour (0xff161616));
    g.fillRoundedRectangle (bounds, 7.0f);
    g.setColour (juce::Colour (0xff333333));
    g.drawRoundedRectangle (bounds, 7.0f, 2.0f);

    const auto slot = bounds.reduced (8.0f, 5.0f);
    const auto knobHeight = slot.getHeight() * 0.46f;
    auto thumb = slot.withHeight (knobHeight);
    if (! isPost)
        thumb.setY (slot.getY());
    else
        thumb.setY (slot.getBottom() - knobHeight);

    if (shouldDrawButtonAsDown)
        thumb = thumb.translated (0.0f, isPost ? -1.5f : 1.5f);

    juce::ColourGradient thumbFill (juce::Colour (0xfff2f2ef), thumb.getX(), thumb.getY(),
                                    juce::Colour (0xff8d8d8d), thumb.getRight(), thumb.getBottom(), false);
    g.setGradientFill (thumbFill);
    g.fillRoundedRectangle (thumb, 7.0f);
    g.setColour (juce::Colour (0xff5c5c5c));
    g.drawRoundedRectangle (thumb, 7.0f, 1.5f);
}

void SendBloomLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour& backgroundColour,
                                                 bool shouldDrawButtonAsHighlighted,
                                                 bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (backgroundColour);

    auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
    auto colour = juce::Colour (0xff070808);
    if (shouldDrawButtonAsHighlighted)
        colour = juce::Colour (0xff111516);
    if (shouldDrawButtonAsDown)
        bounds = bounds.reduced (2.0f).translated (1.0f, 1.0f);

    g.setColour (colour);
    g.fillRoundedRectangle (bounds, 5.0f);
}

void SendBloomLookAndFeel::drawButtonText (juce::Graphics& g, juce::TextButton& button,
                                           bool shouldDrawButtonAsHighlighted,
                                           bool shouldDrawButtonAsDown)
{
    juce::ignoreUnused (shouldDrawButtonAsHighlighted, shouldDrawButtonAsDown);

    g.setColour (cyan);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (8, 2),
                      juce::Justification::centred, 1, 0.82f);
}

void SendBloomLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool isButtonDown,
                                         int buttonX, int buttonY, int buttonW, int buttonH,
                                         juce::ComboBox& box)
{
    juce::ignoreUnused (buttonX, buttonY, buttonW, buttonH);

    const auto bounds = juce::Rectangle<float> (0.0f, 0.0f, static_cast<float> (width), static_cast<float> (height)).reduced (1.0f);
    // Respect per-box theming (the advanced drawer runs a dark panel scheme).
    const auto background = box.isColourSpecified (juce::ComboBox::backgroundColourId)
                                ? box.findColour (juce::ComboBox::backgroundColourId)
                                : juce::Colour (0xffefefef);
    const auto outline = box.isColourSpecified (juce::ComboBox::outlineColourId)
                             ? box.findColour (juce::ComboBox::outlineColourId)
                             : juce::Colours::black;
    const auto arrowColour = box.isColourSpecified (juce::ComboBox::arrowColourId)
                                 ? box.findColour (juce::ComboBox::arrowColourId)
                                 : juce::Colours::black;
    g.setColour (background);
    g.fillRoundedRectangle (bounds, 4.0f);
    g.setColour (outline);
    g.drawRoundedRectangle (bounds, 4.0f, 2.0f);

    const auto cx = static_cast<float> (width - 15);
    const auto cy = static_cast<float> (height) * 0.5f;
    juce::Path arrows;
    arrows.addTriangle (cx - 5.0f, cy - 3.0f, cx + 5.0f, cy - 3.0f, cx, cy - 10.0f);
    arrows.addTriangle (cx - 5.0f, cy + 3.0f, cx + 5.0f, cy + 3.0f, cx, cy + 10.0f);
    g.setColour (isButtonDown ? cyan : arrowColour);
    g.fillPath (arrows);
}

juce::Font SendBloomLookAndFeel::getComboBoxFont (juce::ComboBox&)
{
    return juce::FontOptions (11.0f, juce::Font::bold);
}

} // namespace sendbloom::ui
