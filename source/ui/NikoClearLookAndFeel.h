#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <cmath>

// CLEAR INSTRUMENTS — shared construction, materials and interaction language.
// All geometry is resolution independent. Angles affect only the pointer;
// illumination and highlights stay fixed in screen space.
namespace niko::clear
{
namespace palette
{
inline const juce::Colour warmWhite { 0xfff4f3ef };
inline const juce::Colour ink       { 0xff252a2b };
inline const juce::Colour muted     { 0xff596061 };
inline const juce::Colour signal    { 0xffcd572f };
inline const juce::Colour edge      { 0xffb5b6b0 };
inline const juce::Colour copper    { 0xffab8e67 };
}

inline juce::Font sans (float height, bool bold = false)
{
    return juce::Font (juce::FontOptions ("Helvetica Neue", juce::jmax (12.0f, height),
                                        bold ? juce::Font::bold : juce::Font::plain));
}

inline juce::Font numeric (float height = 14.0f)
{
    return juce::Font (juce::FontOptions (juce::Font::getDefaultMonospacedFontName(),
                                        juce::jmax (14.0f, height), juce::Font::plain));
}

inline void paintPanel (juce::Graphics& g, juce::Rectangle<float> bounds, float radius = 8.0f)
{
    if (bounds.isEmpty()) return;
    g.setColour (palette::ink.withAlpha (0.10f));
    g.fillRoundedRectangle (bounds.translated (0.0f, 2.0f), radius);
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, bounds.getTopLeft(),
                                            palette::warmWhite, bounds.getBottomLeft(), false));
    g.fillRoundedRectangle (bounds, radius);
    g.setColour (palette::edge.withAlpha (0.8f));
    g.drawRoundedRectangle (bounds.reduced (0.5f), radius, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawRoundedRectangle (bounds.reduced (1.5f), juce::jmax (1.0f, radius - 1.0f), 0.8f);
}

inline void paintHeader (juce::Graphics& g, juce::Rectangle<float> bounds,
                         const juce::String& title, const juce::String& subtitle)
{
    // A compact title plaque; never mask the complete circuit field.
    paintPanel (g, bounds, 5.0f);
    auto text = bounds.reduced (14.0f, 5.0f);
    g.setColour (palette::ink);
    g.setFont (sans (26.0f));
    g.drawFittedText (title, text.removeFromTop (31.0f).toNearestInt(), juce::Justification::centredLeft, 1);
    if (subtitle.isNotEmpty())
    {
        g.setColour (palette::muted);
        g.setFont (sans (12.0f));
        g.drawFittedText (subtitle, text.toNearestInt(), juce::Justification::centredLeft, 1);
    }
}

inline void paintScrew (juce::Graphics& g, juce::Point<float> centre, float radius)
{
    auto ring = juce::Rectangle<float> (radius * 2.0f, radius * 2.0f).withCentre (centre);
    g.setColour (palette::ink.withAlpha (0.13f));
    g.fillEllipse (ring.expanded (2.0f).translated (0.0f, 1.0f));
    g.setGradientFill (juce::ColourGradient (juce::Colours::white, ring.getTopLeft(),
                                            juce::Colour (0xff999b93), ring.getBottomRight(), false));
    g.fillEllipse (ring);
    g.setColour (palette::muted.withAlpha (0.65f));
    g.drawEllipse (ring, 0.8f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawEllipse (ring.reduced (radius * 0.19f), 0.8f);
    juce::Path socket;
    for (int i = 0; i <= 48; ++i)
    {
        const float a = static_cast<float> (i) / 48.0f * juce::MathConstants<float>::twoPi;
        const float r = radius * (0.38f + 0.10f * std::cos (a * 6.0f));
        const auto p = centre + juce::Point<float> (std::sin (a), std::cos (a)) * r;
        if (i == 0) socket.startNewSubPath (p); else socket.lineTo (p);
    }
    socket.closeSubPath();
    g.setColour (palette::ink);
    g.fillPath (socket);
    g.setColour (palette::muted);
    g.strokePath (socket, juce::PathStrokeType (0.35f));
}

inline void paintChassisFallback (juce::Graphics& g, juce::Rectangle<float> bounds)
{
    if (bounds.getWidth() < 30.0f || bounds.getHeight() < 30.0f) return;
    juce::Graphics::ScopedSaveState state (g);
    g.reduceClipRegion (bounds.toNearestInt());
    const auto shell = bounds.reduced (5.0f);
    const auto board = shell.reduced (14.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffe8e7e1), bounds.getTopLeft(),
                                            juce::Colour (0xffd6d6cf), bounds.getBottomRight(), false));
    g.fillRoundedRectangle (shell, 17.0f);
    g.setColour (palette::warmWhite);
    g.fillRoundedRectangle (board, 7.0f);

    // Fine, sparse copper paths make the white substrate visible between islands.
    // They are construction detail, never controls or signal metering.
    for (int i = 0; i < 19; ++i)
    {
        const float fraction = static_cast<float> (i + 1) / 20.0f;
        const float x = board.getX() + fraction * board.getWidth();
        const float y = board.getY() + 12.0f + static_cast<float> ((i * 37) % 89);
        const float endY = board.getBottom() - 14.0f - static_cast<float> ((i * 19) % 63);
        juce::Path trace;
        trace.startNewSubPath (x, y);
        trace.lineTo (x, board.getCentreY() - 18.0f);
        trace.lineTo (x + 9.0f, board.getCentreY() - 9.0f);
        trace.lineTo (x + 9.0f, endY);
        g.setColour (palette::copper.withAlpha (0.40f));
        g.strokePath (trace, juce::PathStrokeType (0.65f));
        g.drawEllipse (x - 1.7f, y - 1.7f, 3.4f, 3.4f, 0.7f);
        g.drawEllipse (x + 7.3f, endY - 1.7f, 3.4f, 3.4f, 0.7f);
    }
    for (int i = 0; i < 24; ++i)
    {
        const float x = board.getX() + 14.0f + static_cast<float> ((i * 83) % 997) / 997.0f * juce::jmax (1.0f, board.getWidth() - 38.0f);
        const float y = board.getY() + 13.0f + static_cast<float> ((i * 137) % 431) / 431.0f * juce::jmax (1.0f, board.getHeight() - 37.0f);
        const bool chip = i % 5 == 0;
        auto part = juce::Rectangle<float> (x, y, chip ? 12.0f : 9.0f, chip ? 17.0f : 4.0f);
        g.setColour (palette::ink.withAlpha (0.11f));
        g.fillRoundedRectangle (part.translated (0.0f, 1.0f).expanded (0.5f), 1.0f);
        g.setColour (chip ? palette::muted : juce::Colour (0xffc4c3b7));
        g.fillRoundedRectangle (part, 1.0f);
        g.setColour (juce::Colour (0xffe1e1dc));
        g.fillRect (part.removeFromLeft (2.0f));
        g.fillRect (part.removeFromRight (2.0f));
    }
    // White ribbon harness: screen-space illumination stays fixed.
    for (int i = 0; i < 5; ++i)
    {
        juce::Path cable;
        const float offset = static_cast<float> (i) * 3.0f;
        cable.startNewSubPath (board.getX() + 12.0f, board.getBottom() - 29.0f - offset);
        cable.cubicTo (board.getX() + 65.0f, board.getBottom() - 29.0f - offset,
                       board.getX() + 55.0f, board.getBottom() - 64.0f - offset,
                       board.getX() + 110.0f, board.getBottom() - 64.0f - offset);
        g.setColour (palette::ink.withAlpha (0.16f));
        g.strokePath (cable, juce::PathStrokeType (3.3f));
        g.setColour (juce::Colour (0xfffafaf7));
        g.strokePath (cable, juce::PathStrokeType (2.1f));
    }
    g.setColour (juce::Colours::white.withAlpha (0.65f));
    g.drawRoundedRectangle (shell.reduced (1.5f), 16.0f, 3.0f);
    g.setColour (palette::muted.withAlpha (0.35f));
    g.drawRoundedRectangle (shell.reduced (5.0f), 12.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.75f));
    g.drawRoundedRectangle (shell.reduced (8.0f), 10.0f, 1.5f);
    for (auto p : { shell.getTopLeft() + juce::Point<float> (13, 13),
                    shell.getTopRight() + juce::Point<float> (-13, 13),
                    shell.getBottomLeft() + juce::Point<float> (13, -13),
                    shell.getBottomRight() + juce::Point<float> (-13, -13) })
        paintScrew (g, p, 5.0f);
}

class LookAndFeel final : public juce::LookAndFeel_V4
{
public:
    LookAndFeel()
    {
        setColour (juce::Label::textColourId, palette::ink);
        setColour (juce::Slider::textBoxTextColourId, palette::ink);
        setColour (juce::Slider::textBoxBackgroundColourId, palette::warmWhite);
        setColour (juce::Slider::textBoxOutlineColourId, palette::edge);
        setColour (juce::Slider::thumbColourId, palette::warmWhite);
        setColour (juce::Slider::trackColourId, palette::ink);
        setColour (juce::Slider::backgroundColourId, palette::edge);
        setColour (juce::TextButton::textColourOffId, palette::ink);
        setColour (juce::TextButton::textColourOnId, palette::ink);
        setColour (juce::ComboBox::textColourId, palette::ink);
        setColour (juce::ComboBox::backgroundColourId, palette::warmWhite);
        setColour (juce::PopupMenu::backgroundColourId, palette::warmWhite);
        setColour (juce::PopupMenu::textColourId, palette::ink);
        setColour (juce::PopupMenu::highlightedBackgroundColourId, juce::Colour (0xffe6e2d9));
        setColour (juce::PopupMenu::highlightedTextColourId, palette::ink);
        setColour (juce::TextEditor::textColourId, palette::ink);
        setColour (juce::TextEditor::backgroundColourId, palette::warmWhite);
        setColour (juce::TextEditor::highlightColourId, palette::signal.withAlpha (0.25f));
        setColour (juce::TextEditor::highlightedTextColourId, palette::ink);
        setColour (juce::TextEditor::focusedOutlineColourId, palette::signal);
        setColour (juce::TooltipWindow::backgroundColourId, palette::warmWhite);
        setColour (juce::TooltipWindow::textColourId, palette::ink);
    }

    // Use for actual component resizing, not display/backing scale or a parent transform.
    // Call on the message thread before relaying out controls. Invalid values are ignored.
    void setScaleFactor (float factor)
    {
        if (std::isfinite (factor) && factor > 0.0f) scaleFactor = factor;
    }

    // Optional vertical strip. Each square frame must have identical fixed lighting.
    // Frame 0 and frame N-1 correspond to the slider's minimum and maximum angles.
    void setKnobFilmstrip (juce::Image image, int frames)
    {
        knobStrip = std::move (image);
        stripFrames = frames > 1 && knobStrip.isValid()
                   && knobStrip.getHeight() % frames == 0
                   && knobStrip.getWidth() == knobStrip.getHeight() / frames ? frames : 0;
    }

    // Borders are SOURCE pixels, scale is source pixels per logical UI pixel.
    // Content insets are nominal logical UI pixels and remain fixed at any width.
    void setDisplayImage (juce::Image image, juce::BorderSize<int> sourceBorder = { 8, 8, 8, 8 },
                          float sourceScale = 2.0f,
                          juce::BorderSize<float> contentInsets = { 3.0f, 5.0f, 3.0f, 5.0f })
    {
        displaySkin = makeSkin (std::move (image), sourceBorder, sourceScale, contentInsets);
    }

    void setButtonImages (juce::Image up, juce::Image down,
                          juce::BorderSize<int> sourceBorder = { 8, 8, 8, 8 }, float sourceScale = 2.0f,
                          juce::BorderSize<float> contentInsets = { 3.0f, 7.0f, 3.0f, 7.0f })
    {
        buttonUp = makeSkin (std::move (up), sourceBorder, sourceScale, contentInsets);
        buttonDown = makeSkin (std::move (down), sourceBorder, sourceScale, contentInsets);
    }

    void setSelectorImage (juce::Image image, juce::BorderSize<int> sourceBorder = { 8, 8, 8, 8 },
                           float sourceScale = 2.0f,
                           juce::BorderSize<float> contentInsets = { 2.0f, 8.0f, 2.0f, 28.0f })
    {
        selectorSkin = makeSkin (std::move (image), sourceBorder, sourceScale, contentInsets);
    }

    void setSwitchImages(juce::Image off, juce::Image on)
    {
        const bool valid = off.isValid() && on.isValid() && off.getBounds() == on.getBounds();
        switchOff = valid ? std::move(off) : juce::Image();
        switchOn = valid ? std::move(on) : juce::Image();
    }

    // Upright physical cap for horizontal single-value faders only; never rotate lighting.
    void setFaderImage (juce::Image image) { faderThumb = std::move (image); }

    // Does not change editing mode, listeners, parameter bindings, focus or input handling.
    void configureValueLabel (juce::Label& label, float nominalFontHeight = 14.0f)
    {
        const auto height = std::isfinite (nominalFontHeight) ? juce::jmax (14.0f, nominalFontHeight) : 14.0f;
        label.getProperties().set ("niko.value", true);
        label.getProperties().set ("niko.valueFontSize", height);
        label.setFont (numeric (height));
        label.setColour (juce::Label::textWhenEditingColourId, valueInk);
        label.setColour (juce::Label::backgroundWhenEditingColourId, valueBackground);
        label.setColour (juce::Label::outlineWhenEditingColourId, palette::signal);
        label.setColour (juce::TextEditor::textColourId, valueInk);
        label.setColour (juce::TextEditor::backgroundColourId, valueBackground);
        label.setColour (juce::TextEditor::highlightColourId, juce::Colour (0xff80422a));
        label.setColour (juce::TextEditor::highlightedTextColourId, juce::Colours::white);
        label.setColour (juce::CaretComponent::caretColourId, valueInk);
    }

    // Bounds/result are in the caller's actual component coordinates. The returned
    // content rectangle lets custom Mix/Delay displays retain their own drawing and input.
    juce::Rectangle<float> paintValueWindow (juce::Graphics& g, juce::Rectangle<float> bounds,
                                             bool enabled = true, bool focused = false)
    {
        juce::Graphics::ScopedSaveState state (g);
        if (bounds.isEmpty()) return bounds;
        g.addTransform (juce::AffineTransform::scale (scaleFactor));
        const auto logical = bounds.transformedBy (juce::AffineTransform::scale (1.0f / scaleFactor));
        if (! enabled) g.beginTransparencyLayer (0.45f);
        if (displaySkin.image.isValid()) drawSkin (g, displaySkin, logical);
        else
        {
            const auto rim = logical.reduced (0.5f);
            g.setColour (juce::Colour (0xffb5b5aa));
            g.fillRoundedRectangle (rim, 2.0f);
            const auto lens = rim.reduced (1.5f);
            g.setGradientFill (juce::ColourGradient (juce::Colour (0xff101a1a), lens.getTopLeft(),
                                                    juce::Colour (0xff34403c), lens.getBottomRight(), false));
            g.fillRoundedRectangle (lens, 1.0f);
            g.setColour (juce::Colours::black.withAlpha (0.35f));
            g.fillRect (lens.withHeight (juce::jmin (2.0f, lens.getHeight())));
            g.setColour (juce::Colours::white.withAlpha (0.08f));
            g.drawLine (lens.getX() + 1.0f, lens.getY() + 1.0f, lens.getRight() - 1.0f, lens.getY() + 1.0f, 0.8f);
        }
        if (focused)
        {
            g.setColour (palette::signal);
            g.drawRoundedRectangle (logical.reduced (0.5f), 2.0f, 1.0f);
        }
        if (! enabled) g.endTransparencyLayer();
        return insetBounds (bounds, displaySkin.contentInsets, scaleFactor);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height,
                           float position, float startAngle, float endAngle, juce::Slider& slider) override
    {
        juce::Graphics::ScopedSaveState state (g);
        auto area = juce::Rectangle<float> (static_cast<float> (x), static_cast<float> (y),
                                           static_cast<float> (width), static_cast<float> (height));
        const float size = juce::jmax (1.0f, juce::jmin (area.getWidth(), area.getHeight()) - scaled (10.0f));
        auto skirt = juce::Rectangle<float> (size, size).withCentre (area.getCentre());
        const auto centre = skirt.getCentre();
        const float angle = startAngle + juce::jlimit (0.0f, 1.0f, position) * (endAngle - startAngle);
        if (! slider.isEnabled()) g.beginTransparencyLayer (0.46f);
        // Hardware is its own pointer affordance. Keyboard focus uses two
        // small brackets at the collar, not an oversized software hover ring.
        if (slider.hasKeyboardFocus(true))
        {
            const float reach = size * .455f;
            g.setColour(palette::signal.withAlpha(.9f));
            for (const float side : { -1.0f, 1.0f })
            {
                const auto px = centre.x + side * reach;
                g.drawLine(px, centre.y - 6.0f, px, centre.y + 6.0f, 1.1f);
                g.drawLine(px, centre.y - 6.0f, px - side * 3.0f, centre.y - 6.0f, 1.1f);
                g.drawLine(px, centre.y + 6.0f, px - side * 3.0f, centre.y + 6.0f, 1.1f);
            }
        }
        const bool detented = static_cast<bool> (slider.getProperties()["niko.detented"]);
        if (detented)
        {
            const double step = slider.getInterval();
            const int count = step > 0.0 ? juce::jlimit (2, 16, juce::roundToInt ((slider.getMaximum() - slider.getMinimum()) / step) + 1) : 7;
            for (int i = 0; i < count; ++i)
            {
                const float a = startAngle + static_cast<float> (i) / static_cast<float> (count - 1) * (endAngle - startAngle);
                const auto p = centre + juce::Point<float> (std::sin (a), -std::cos (a)) * (size * 0.48f);
                g.setColour (palette::muted);
                g.fillEllipse (p.x - 1.1f, p.y - 1.1f, 2.2f, 2.2f);
            }
        }
        if (stripFrames > 1)
        {
            // Study C occupies about 86% of its square frame. Shadow is local
            // to the base, shifted away from the fixed upper-left key light.
            // The alpha-clean sprite supplies geometry; no rectangular catcher.
            const float body = size * .86f;
            for (int i = 4; i >= 0; --i)
            {
                const float spread = static_cast<float>(i) * .65f;
                const auto ground = juce::Rectangle<float>(body + spread * 2.0f,
                                                            body * .88f + spread * 1.6f)
                                     .withCentre(centre.translated(size * .025f, size * .045f));
                g.setColour(palette::ink.withAlpha(i == 0 ? .11f : .018f));
                g.fillEllipse(ground);
            }
            g.setColour(juce::Colours::black.withAlpha(.14f));
            g.fillEllipse(juce::Rectangle<float>(body * .92f, body * .80f)
                           .withCentre(centre.translated(size * .01f, size * .028f)));
            const int frame = juce::roundToInt (position * static_cast<float> (stripFrames - 1));
            const int frameHeight = knobStrip.getHeight() / stripFrames;
            g.setOpacity (1.0f); // Images otherwise inherit the contact-shadow brush alpha.
            g.drawImage (knobStrip, juce::roundToInt (skirt.getX()), juce::roundToInt (skirt.getY()),
                         juce::roundToInt (size), juce::roundToInt (size), 0,
                         juce::jlimit (0, stripFrames - 1, frame) * frameHeight, knobStrip.getWidth(), frameHeight);
            if (! slider.isEnabled()) g.endTransparencyLayer();
            return;
        }
        g.setColour (palette::ink.withAlpha (0.10f));
        g.fillEllipse (skirt.reduced (1.0f).translated (0.0f, size * 0.045f));
        g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.67f), skirt.getTopLeft(),
                                                palette::edge.withAlpha (0.30f), skirt.getBottomRight(), false));
        g.fillEllipse (skirt.reduced (1.0f));
        g.setColour (palette::muted.withAlpha (0.5f));
        g.drawEllipse (skirt.reduced (1.0f), 0.7f);
        g.setColour (juce::Colours::white.withAlpha (0.9f));
        g.drawEllipse (skirt.reduced (3.0f), 0.9f);
        auto collar = skirt.reduced (size * 0.105f).translated (0.0f, size * 0.035f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff656a68), collar.getTopLeft(),
                                                juce::Colour (0xff242928), collar.getBottomRight(), false));
        g.fillEllipse (collar);
        for (int i = 0; i < 52; ++i)
        {
            const float a = static_cast<float> (i) / 52.0f * juce::MathConstants<float>::twoPi;
            const auto v = juce::Point<float> (std::sin (a), std::cos (a));
            const auto p1 = collar.getCentre() + v * (collar.getWidth() * 0.43f);
            const auto p2 = collar.getCentre() + v * (collar.getWidth() * 0.49f);
            g.setColour (juce::Colours::white.withAlpha (0.10f + 0.13f * (1.0f - std::cos (a)) * 0.5f));
            g.drawLine ({ p1, p2 }, 0.8f);
        }
        auto cap = skirt.reduced (size * 0.14f).translated (0.0f, -size * 0.025f);
        g.setColour (palette::ink);
        g.fillEllipse (cap.expanded (1.0f));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffcfcf9), cap.getTopLeft(),
                                                juce::Colour (0xffddded8), cap.getBottomRight(), false));
        g.fillEllipse (cap);
        g.setColour (juce::Colours::white.withAlpha (0.90f));
        g.drawEllipse (cap.reduced (1.0f), 0.8f);
        const auto direction = juce::Point<float> (std::sin (angle), -std::cos (angle));
        const auto p1 = cap.getCentre() + direction * (cap.getWidth() * 0.19f);
        const auto p2 = cap.getCentre() + direction * (cap.getWidth() * 0.43f);
        g.setColour (juce::Colours::white.withAlpha (0.75f));
        g.drawLine ({ p1.translated (0.7f, 0.7f), p2.translated (0.7f, 0.7f) }, juce::jmax (1.8f, size * 0.029f));
        g.setColour (palette::ink);
        g.drawLine ({ p1, p2 }, juce::jmax (1.6f, size * 0.026f));
        if (! slider.isEnabled()) g.endTransparencyLayer();
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&,
                                bool hover, bool down) override
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (scaleFactor));
        const auto b = logicalBounds (button).reduced (0.5f);
        if (! button.isEnabled()) g.beginTransparencyLayer (0.45f);
        const auto& skin = (down || button.getToggleState()) && buttonDown.image.isValid() ? buttonDown : buttonUp;
        if (skin.image.isValid()) drawSkin (g, skin, b);
        else paintPanel (g, b, 4.0f);
        if (button.getToggleState())
        {
            g.setColour (palette::signal);
            g.fillRoundedRectangle (b.getX() + 5.0f, b.getCentreY() - 2.0f, 2.0f, 4.0f, 0.7f);
        }
        if (button.hasKeyboardFocus (true) || hover || down)
        {
            g.setColour ((button.hasKeyboardFocus (true) ? palette::signal : palette::muted).withAlpha (0.7f));
            g.drawRoundedRectangle (b.reduced (1.0f), 3.0f, button.hasKeyboardFocus (true) ? 1.5f : 0.7f);
        }
        if (! button.isEnabled()) g.endTransparencyLayer();
    }

    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool, bool) override
    {
        g.setFont (scaledSans (12.0f, button.getToggleState()));
        g.setColour (palette::ink.withAlpha (button.isEnabled() ? 1.0f : 0.45f));
        g.drawFittedText (button.getButtonText(), insetBounds (button.getLocalBounds().toFloat(), buttonUp.contentInsets, scaleFactor).toNearestInt(),
                          juce::Justification::centred, 1);
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool hover, bool down) override
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (scaleFactor));
        const auto area = logicalBounds (button);
        if (area.isEmpty()) return;
        const bool compact = static_cast<bool> (button.getProperties()["niko.switchCompact"])
                          || area.getHeight() < 36.0f || area.getWidth() < 124.0f;
        const float nominalHeight = compact ? 24.0f : 34.0f;
        const float h = juce::jmin (nominalHeight, juce::jmax (0.0f, area.getHeight() - 2.0f));
        const float w = juce::jmin (area.getWidth(), h * (58.0f / 34.0f));
        const auto hardware = juce::Rectangle<float> (w, h).withPosition (0.0f, area.getCentreY() - h * 0.5f);
        const auto textArea = area.withTrimmedLeft (juce::jmin (area.getWidth(), w + 4.0f)).reduced (1.0f, 0.0f);
        const bool on = button.getToggleState();
        if (! button.isEnabled()) g.beginTransparencyLayer (0.45f);
        if (switchOff.isValid() && switchOn.isValid())
        {
            g.setOpacity (1.0f);
            g.drawImage (on ? switchOn : switchOff, hardware, juce::RectanglePlacement::stretchToFit, false);
        }
        else
        {
            const auto track = hardware.reduced (2.0f, h * 0.22f);
            g.setColour (palette::ink);
            g.fillRoundedRectangle (track, 2.0f);
            auto cap = juce::Rectangle<float> (track.getWidth() * 0.42f, track.getHeight() - 2.0f)
                         .withPosition (on ? track.getRight() - track.getWidth() * 0.42f - 1.0f : track.getX() + 1.0f, track.getY() + 1.0f);
            g.setGradientFill (juce::ColourGradient (juce::Colours::white, cap.getTopLeft(), palette::edge, cap.getBottomRight(), false));
            g.fillRoundedRectangle (cap, 1.0f);
            g.setColour (palette::muted);
            g.drawLine (cap.getCentreX(), cap.getY() + 2.0f, cap.getCentreX(), cap.getBottom() - 2.0f, 0.8f);
        }
        g.setColour (palette::warmWhite);
        g.fillRect (textArea);
        g.setColour (palette::ink);
        g.setFont (sans (12.0f, true));
        g.drawFittedText (button.getButtonText(), (compact ? textArea : textArea.withHeight (17.0f)).toNearestInt(),
                          juce::Justification::centredLeft, 1, 1.0f);
        if (! compact)
        {
            g.setColour (palette::muted);
            g.setFont (sans (12.0f));
            g.drawText (on ? "ON" : "OFF", textArea.withTrimmedTop (17.0f), juce::Justification::centredLeft, false);
        }
        if (on)
        {
            g.setColour (palette::signal);
            g.fillEllipse (hardware.getRight() - 4.0f, hardware.getBottom() - 3.0f, 2.5f, 2.5f);
        }
        if (button.hasKeyboardFocus (true) || hover || down)
        {
            g.setColour (palette::signal.withAlpha (button.hasKeyboardFocus (true) ? 0.9f : 0.5f));
            g.drawRoundedRectangle (hardware.reduced (1.0f), 1.0f, 1.0f);
        }
        if (! button.isEnabled()) g.endTransparencyLayer();
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override { return scaledSans (13.0f); }
    juce::Font getPopupMenuFont() override { return scaledSans (13.0f); }
    void drawComboBox (juce::Graphics& g, int width, int height, bool down,
                       int, int, int, int, juce::ComboBox& box) override
    {
        juce::Graphics::ScopedSaveState state (g);
        g.addTransform (juce::AffineTransform::scale (scaleFactor));
        auto b = juce::Rectangle<float> (static_cast<float> (width) / scaleFactor, static_cast<float> (height) / scaleFactor).reduced (0.5f);
        if (! box.isEnabled()) g.beginTransparencyLayer (0.45f);
        if (selectorSkin.image.isValid()) drawSkin (g, selectorSkin, b);
        else paintPanel (g, b, 4.0f);
        g.setColour ((box.hasKeyboardFocus (true) ? palette::signal : palette::muted).withAlpha (box.isEnabled() ? 1.0f : 0.4f));
        if (down || box.hasKeyboardFocus (true)) g.drawRoundedRectangle (b, 4.0f, 1.5f);
        const float cx = static_cast<float> (width) / scaleFactor - 13.0f, cy = static_cast<float> (height) / scaleFactor * 0.5f;
        juce::Path arrow;
        arrow.startNewSubPath (cx - 3.0f, cy - 1.5f);
        arrow.lineTo (cx, cy + 1.5f);
        arrow.lineTo (cx + 3.0f, cy - 1.5f);
        g.strokePath (arrow, juce::PathStrokeType (1.4f));
        if (! box.isEnabled()) g.endTransparencyLayer();
    }
    void positionComboBoxText (juce::ComboBox& box, juce::Label& label) override
    {
        label.setBounds (insetBounds (box.getLocalBounds().toFloat(), selectorSkin.contentInsets, scaleFactor).toNearestInt());
        label.setFont (sans (13.0f)); // Store nominal height; getLabelFont applies scale once.
    }
    juce::Font getLabelFont (juce::Label& label) override
    {
        if (! static_cast<bool> (label.getProperties()["niko.value"])) return scaledSans (label.getFont().getHeight());
        const auto requested = static_cast<float> (label.getProperties().getWithDefault ("niko.valueFontSize", 14.0f));
        const auto height = std::isfinite (requested) ? juce::jmax (14.0f, requested) : 14.0f;
        return numeric (height).withHeight (scaled (height));
    }
    void drawLabel (juce::Graphics& g, juce::Label& label) override
    {
        juce::Graphics::ScopedSaveState state (g);
        const bool value = static_cast<bool> (label.getProperties()["niko.value"]);
        const auto bounds = label.getLocalBounds().toFloat();
        auto content = insetBounds (bounds, asFloatBorder (label.getBorderSize()), scaleFactor);
        if (value)
            content = content.getIntersection (paintValueWindow (g, bounds, label.isEnabled(), label.isBeingEdited() || label.hasKeyboardFocus (true)));
        else
        {
            g.setColour (label.findColour (juce::Label::backgroundColourId));
            g.fillRect (bounds);
        }
        if (! label.isBeingEdited())
        {
            g.setFont (getLabelFont (label));
            g.setColour ((value ? valueInk : palette::ink).withAlpha (label.isEnabled() ? 1.0f : 0.45f));
            g.drawFittedText (label.getText(), content.toNearestInt(), label.getJustificationType(), 1);
        }
    }
    juce::Label* createSliderTextBox (juce::Slider& slider) override
    {
        auto* label = juce::LookAndFeel_V4::createSliderTextBox (slider);
        configureValueLabel (*label);
        label->setJustificationType (juce::Justification::centred);
        return label;
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float position, float minimum, float maximum,
                           juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        // Preserve JUCE's range-slider semantics and its additional thumbs.
        if (style != juce::Slider::LinearHorizontal && style != juce::Slider::LinearVertical)
        {
            juce::LookAndFeel_V4::drawLinearSlider (g, x, y, width, height, position, minimum, maximum, style, slider);
            return;
        }
        juce::Graphics::ScopedSaveState state (g);
        const bool horizontal = slider.isHorizontal();
        const auto a = juce::Point<float> (static_cast<float> (x), static_cast<float> (y));
        auto track = horizontal ? juce::Rectangle<float> (a.x, a.y + static_cast<float> (height) * 0.5f - scaled (2.0f), static_cast<float> (width), scaled (4.0f))
                                : juce::Rectangle<float> (a.x + static_cast<float> (width) * 0.5f - scaled (2.0f), a.y, scaled (4.0f), static_cast<float> (height));
        g.setColour (palette::ink.withAlpha (slider.isEnabled() ? 0.60f : 0.25f));
        g.fillRoundedRectangle (track, scaled (2.0f));
        auto cap = juce::Rectangle<float> (scaled (horizontal ? 13.0f : 24.0f), scaled (horizontal ? 24.0f : 13.0f))
                     .withCentre (horizontal ? juce::Point<float> (position, track.getCentreY()) : juce::Point<float> (track.getCentreX(), position));
        if (horizontal && faderThumb.isValid())
        {
            if (! slider.isEnabled()) g.beginTransparencyLayer (0.45f);
            g.setOpacity (1.0f);
            g.drawImage (faderThumb, cap, juce::RectanglePlacement::centred, false);
            if (! slider.isEnabled()) g.endTransparencyLayer();
        }
        else
        {
            {
                juce::Graphics::ScopedSaveState capState (g);
                g.addTransform (juce::AffineTransform::scale (scaleFactor));
                paintPanel (g, cap.transformedBy (juce::AffineTransform::scale (1.0f / scaleFactor)), 3.0f);
            }
            g.setColour (slider.hasKeyboardFocus (true) ? palette::signal : palette::ink);
            if (horizontal) g.drawLine (cap.getCentreX(), cap.getY() + scaled (5.0f), cap.getCentreX(), cap.getBottom() - scaled (5.0f), scaled (1.0f));
            else g.drawLine (cap.getX() + scaled (5.0f), cap.getCentreY(), cap.getRight() - scaled (5.0f), cap.getCentreY(), scaled (1.0f));
        }
        g.setColour (slider.hasKeyboardFocus (true) ? palette::signal : palette::ink);
        if (slider.hasKeyboardFocus (true) || slider.isMouseOverOrDragging())
            g.drawRoundedRectangle (cap.expanded (scaled (2.0f)), scaled (4.0f), scaled (1.0f));
    }

private:
    struct SurfaceSkin
    {
        juce::Image image;
        juce::BorderSize<int> sourceBorder;
        float sourceScale = 2.0f;
        juce::BorderSize<float> contentInsets;
    };
    inline static const juce::Colour valueInk { 0xffede6cf }, valueBackground { 0xff17221f };
    static SurfaceSkin makeSkin (juce::Image image, juce::BorderSize<int> border, float sourceScale,
                                  juce::BorderSize<float> content)
    {
        return { std::move (image), border,
                 std::isfinite (sourceScale) && sourceScale > 0.0f ? sourceScale : 2.0f, content };
    }
    static juce::BorderSize<float> asFloatBorder (juce::BorderSize<int> border)
    {
        return { static_cast<float> (border.getTop()), static_cast<float> (border.getLeft()),
                 static_cast<float> (border.getBottom()), static_cast<float> (border.getRight()) };
    }
    static juce::Rectangle<float> insetBounds (juce::Rectangle<float> bounds,
                                              juce::BorderSize<float> border, float factor)
    {
        const auto valid = [factor] (float v) { return std::isfinite (v) ? juce::jmax (0.0f, v * factor) : 0.0f; };
        const float left = juce::jmin (bounds.getWidth(), valid (border.getLeft()));
        const float right = juce::jmin (bounds.getWidth() - left, valid (border.getRight()));
        const float top = juce::jmin (bounds.getHeight(), valid (border.getTop()));
        const float bottom = juce::jmin (bounds.getHeight() - top, valid (border.getBottom()));
        return { bounds.getX() + left, bounds.getY() + top,
                 juce::jmax (0.0f, bounds.getWidth() - left - right), juce::jmax (0.0f, bounds.getHeight() - top - bottom) };
    }
    static void drawSkin (juce::Graphics& g, const SurfaceSkin& skin, juce::Rectangle<float> target)
    {
        if (! skin.image.isValid() || target.isEmpty()) return;
        juce::Graphics::ScopedSaveState state (g);
        g.setOpacity (1.0f);
        const int width = skin.image.getWidth(), height = skin.image.getHeight();
        const int left = juce::jlimit (0, (width - 1) / 2, skin.sourceBorder.getLeft());
        const int right = juce::jlimit (0, (width - 1) / 2, skin.sourceBorder.getRight());
        const int top = juce::jlimit (0, (height - 1) / 2, skin.sourceBorder.getTop());
        const int bottom = juce::jlimit (0, (height - 1) / 2, skin.sourceBorder.getBottom());
        const float fitX = juce::jmin (1.0f, target.getWidth() * skin.sourceScale / static_cast<float> (juce::jmax (1, left + right)));
        const float fitY = juce::jmin (1.0f, target.getHeight() * skin.sourceScale / static_cast<float> (juce::jmax (1, top + bottom)));
        const int sx[] { 0, left, width - right, width }, sy[] { 0, top, height - bottom, height };
        const float dx[] { target.getX(), target.getX() + static_cast<float> (left) / skin.sourceScale * fitX,
                           target.getRight() - static_cast<float> (right) / skin.sourceScale * fitX, target.getRight() };
        const float dy[] { target.getY(), target.getY() + static_cast<float> (top) / skin.sourceScale * fitY,
                           target.getBottom() - static_cast<float> (bottom) / skin.sourceScale * fitY, target.getBottom() };
        for (int row = 0; row < 3; ++row)
            for (int col = 0; col < 3; ++col)
            {
                const juce::Rectangle<int> source (sx[col], sy[row], sx[col + 1] - sx[col], sy[row + 1] - sy[row]);
                const juce::Rectangle<float> dest (dx[col], dy[row], dx[col + 1] - dx[col], dy[row + 1] - dy[row]);
                if (! source.isEmpty() && ! dest.isEmpty())
                    g.drawImage (skin.image.getClippedImage (source), dest, juce::RectanglePlacement::stretchToFit, false);
            }
    }
    float scaled (float value) const { return value * scaleFactor; }
    juce::Font scaledSans (float height, bool bold = false) const { return sans (height, bold).withHeight (scaled (juce::jmax (12.0f, height))); }
    juce::Font scaledNumeric() const { return numeric().withHeight (scaled (14.0f)); }
    juce::Rectangle<float> logicalBounds (const juce::Component& component) const
    {
        return component.getLocalBounds().toFloat().transformedBy (juce::AffineTransform::scale (1.0f / scaleFactor));
    }
    float scaleFactor = 1.0f;
    juce::Image knobStrip;
    juce::Image faderThumb;
    juce::Image switchOff, switchOn;
    SurfaceSkin displaySkin { {}, { 8, 8, 8, 8 }, 2.0f, { 3.0f, 5.0f, 3.0f, 5.0f } };
    SurfaceSkin buttonUp { {}, { 8, 8, 8, 8 }, 2.0f, { 3.0f, 7.0f, 3.0f, 7.0f } }, buttonDown;
    SurfaceSkin selectorSkin { {}, { 8, 8, 8, 8 }, 2.0f, { 2.0f, 8.0f, 2.0f, 28.0f } };
    int stripFrames = 0;
};
} // namespace niko::clear
