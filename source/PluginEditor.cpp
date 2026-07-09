#include "PluginEditor.h"
#include "FactoryPresets.h"
#include <BinaryData.h>

#include <cmath>

namespace sendbloom
{

namespace
{

constexpr int kEditorWidth = 420;
constexpr int kEditorHeight = 780;

juce::String upperPresetName (int index)
{
    if (index == 0)
        return "INITIAL PATCH";

    return FactoryPresets::getPresetName (index).toUpperCase();
}

juce::String formatSignedDbFromNorm (double value)
{
    const auto db = (0.5 - value) * 18.0;
    if (std::abs (db) < 0.005)
        return "-0.00";

    return juce::String (db, 2);
}

void drawLogo (juce::Graphics& g, juce::Rectangle<float> logo)
{
    juce::Path orange;
    orange.startNewSubPath (logo.getX() + 18.0f, logo.getY());
    orange.lineTo (logo.getRight() - 12.0f, logo.getY());
    orange.lineTo (logo.getRight() - 30.0f, logo.getBottom());
    orange.lineTo (logo.getX(), logo.getBottom());
    orange.closeSubPath();

    g.setColour (juce::Colours::black);
    g.fillRoundedRectangle (logo.expanded (7.0f), 7.0f);
    g.setColour (juce::Colour (0xffff8f25));
    g.fillPath (orange);

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (55.0f, juce::Font::bold));
    g.drawFittedText ("Niko", logo.withTrimmedLeft (18.0f).withWidth (206.0f).toNearestInt(),
                      juce::Justification::centredLeft, 1, 0.82f);

    g.setColour (juce::Colour (0xffff8f25));
    g.fillRect (logo.withTrimmedLeft (224.0f).withTrimmedRight (18.0f));
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (58.0f, juce::Font::bold));
    g.drawFittedText ("FX", logo.withTrimmedLeft (218.0f).toNearestInt(),
                      juce::Justification::centred, 1, 0.8f);
}

void drawCyanFrame (juce::Graphics& g, juce::Rectangle<float> bounds, juce::Colour cyan)
{
    juce::Path frame;
    frame.startNewSubPath (bounds.getX() + 16.0f, bounds.getY());
    frame.lineTo (bounds.getX(), bounds.getY() + 16.0f);
    frame.lineTo (bounds.getX(), bounds.getBottom() - 28.0f);
    frame.lineTo (bounds.getX() + 20.0f, bounds.getBottom());
    frame.lineTo (bounds.getRight() - 42.0f, bounds.getBottom());
    frame.lineTo (bounds.getRight(), bounds.getBottom() - 42.0f);
    frame.lineTo (bounds.getRight(), bounds.getY() + 16.0f);
    frame.lineTo (bounds.getRight() - 18.0f, bounds.getY());

    g.setColour (cyan);
    g.strokePath (frame, juce::PathStrokeType (5.0f, juce::PathStrokeType::mitered));
}

void drawChevronRail (juce::Graphics& g, float x, float y, int count, juce::Colour cyan)
{
    g.setColour (cyan);
    for (int i = 0; i < count; ++i)
    {
        const auto top = y + static_cast<float> (i) * 34.0f;
        juce::Path chevron;
        chevron.startNewSubPath (x, top);
        chevron.lineTo (x + 14.0f, top + 16.0f);
        chevron.lineTo (x + 28.0f, top);
        chevron.lineTo (x + 28.0f, top + 12.0f);
        chevron.lineTo (x + 14.0f, top + 28.0f);
        chevron.lineTo (x, top + 12.0f);
        chevron.closeSubPath();
        g.fillPath (chevron);
    }
}

void drawControlIcons (juce::Graphics& g, juce::Rectangle<int> area)
{
    auto drawIcon = [&g] (juce::Rectangle<int> bounds, const juce::String& text)
    {
        const auto icon = bounds.removeFromTop (24).toFloat();
        g.setColour (juce::Colour (0xff23282b));

        if (text == "SAVE")
        {
            g.fillRoundedRectangle (icon.reduced (7.0f, 2.0f), 2.0f);
            g.setColour (juce::Colours::white);
            g.fillRect (icon.getX() + 12.0f, icon.getY() + 5.0f, 7.0f, 5.0f);
            g.fillEllipse (icon.getX() + 13.0f, icon.getY() + 15.0f, 6.0f, 6.0f);
        }
        else if (text == "NEW")
        {
            juce::Path page;
            page.startNewSubPath (icon.getX() + 8.0f, icon.getY() + 2.0f);
            page.lineTo (icon.getX() + 21.0f, icon.getY() + 2.0f);
            page.lineTo (icon.getX() + 28.0f, icon.getY() + 9.0f);
            page.lineTo (icon.getX() + 28.0f, icon.getBottom() - 2.0f);
            page.lineTo (icon.getX() + 8.0f, icon.getBottom() - 2.0f);
            page.closeSubPath();
            g.strokePath (page, juce::PathStrokeType (3.0f));
        }
        else
        {
            g.fillRect (icon.getX() + 11.0f, icon.getY() + 7.0f, 17.0f, 18.0f);
            g.fillRect (icon.getX() + 9.0f, icon.getY() + 4.0f, 21.0f, 3.0f);
            g.fillRect (icon.getX() + 15.0f, icon.getY() + 1.0f, 9.0f, 3.0f);
        }

        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (10.0f, juce::Font::bold));
        g.drawText (text, bounds, juce::Justification::centredTop);
    };

    const auto width = area.getWidth() / 3;
    drawIcon (area.removeFromLeft (width), "SAVE");
    drawIcon (area.removeFromLeft (width), "NEW");
    drawIcon (area, "DELETE");
}

void fillTriangle (juce::Graphics& g, float x1, float y1, float x2, float y2, float x3, float y3)
{
    juce::Path triangle;
    triangle.addTriangle (x1, y1, x2, y2, x3, y3);
    g.fillPath (triangle);
}

void drawAdvancedFanout (juce::Graphics& g, juce::Colour cyan)
{
    juce::Path openPanel;
    openPanel.startNewSubPath (236.0f, 560.0f);
    openPanel.lineTo (398.0f, 560.0f);
    openPanel.lineTo (398.0f, 725.0f);
    openPanel.lineTo (370.0f, 754.0f);
    openPanel.lineTo (230.0f, 754.0f);
    openPanel.lineTo (230.0f, 578.0f);
    openPanel.closeSubPath();
    g.setColour (juce::Colours::black);
    g.fillPath (openPanel);
    g.setColour (cyan);
    g.strokePath (openPanel, juce::PathStrokeType (4.0f));

    g.setFont (juce::FontOptions (17.0f, juce::Font::bold));
    g.drawText ("ADVANCED", 242, 570, 132, 24, juce::Justification::centred);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText ("GATE SENS", 244, 606, 76, 18, juce::Justification::centred);
    g.drawText ("SEND FEEL", 320, 606, 70, 18, juce::Justification::centred);
    g.drawRoundedRectangle ({ 258.0f, 632.0f, 42.0f, 42.0f }, 21.0f, 3.0f);
    g.drawRoundedRectangle ({ 324.0f, 633.0f, 54.0f, 24.0f }, 3.0f, 2.0f);
    g.drawText ("FIRM", 324, 633, 54, 24, juce::Justification::centred);
    g.drawText ("32K COLOR", 250, 684, 130, 20, juce::Justification::centred);
    g.drawText ("EXTENDED STEREO", 248, 708, 134, 20, juce::Justification::centred);
    g.drawText ("DIRT OS", 248, 732, 134, 20, juce::Justification::centred);
}

bool isParamOn (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* value = apvts.getRawParameterValue (id))
        return value->load() > 0.5f;

    return false;
}

int getChoiceIndex (juce::AudioProcessorValueTreeState& apvts, const char* id)
{
    if (auto* value = apvts.getRawParameterValue (id))
        return static_cast<int> (std::round (value->load()));

    return 0;
}

void drawDarkPressedOverlay (juce::Graphics& g)
{
    const auto body = juce::Rectangle<float> (64.0f, 568.0f, 50.0f, 50.0f);
    juce::ColourGradient fill (juce::Colour (0xff15191f), body.getX(), body.getY(),
                               juce::Colour (0xff59616c), body.getRight(), body.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (body.translated (1.5f, 3.0f).reduced (2.0f), 6.0f);
    g.setColour (juce::Colours::black.withAlpha (0.8f));
    g.drawRoundedRectangle (body.translated (1.5f, 3.0f).reduced (1.0f), 6.0f, 4.0f);
    g.setColour (juce::Colours::white.withAlpha (0.10f));
    g.drawRoundedRectangle (body.translated (1.5f, 3.0f).reduced (8.0f), 4.0f, 1.5f);
}

void drawGatePositionOverlay (juce::Graphics& g, bool post)
{
    const auto slot = juce::Rectangle<float> (143.0f, 560.0f, 27.0f, 64.0f);
    g.setColour (juce::Colour (0xff151515));
    g.fillRoundedRectangle (slot, 8.0f);
    g.setColour (juce::Colour (0xff444444));
    g.drawRoundedRectangle (slot, 8.0f, 2.0f);

    auto thumb = juce::Rectangle<float> (148.0f, post ? 590.0f : 567.0f, 17.0f, 26.0f);
    juce::ColourGradient fill (juce::Colour (0xfff3f3ee), thumb.getX(), thumb.getY(),
                               juce::Colour (0xff7c7c7c), thumb.getRight(), thumb.getBottom(), false);
    g.setGradientFill (fill);
    g.fillRoundedRectangle (thumb, 8.0f);
    g.setColour (juce::Colour (0xff5a5a5a));
    g.drawRoundedRectangle (thumb, 8.0f, 1.5f);
}

void drawFootswitchPressedOverlay (juce::Graphics& g)
{
    const auto centre = juce::Point<float> (137.0f, 696.0f);
    const auto ring = juce::Rectangle<float> (centre.x - 46.0f, centre.y - 39.0f, 92.0f, 88.0f);
    g.setColour (juce::Colours::black.withAlpha (0.56f));
    g.fillEllipse (ring.translated (0.0f, 9.0f));

    for (int i = 0; i < 4; ++i)
    {
        const auto f = static_cast<float> (i);
        const auto r = ring.reduced (f * 7.0f).translated (0.0f, 5.0f);
        juce::ColourGradient fill (juce::Colour (0xffc9c9c6).darker (0.08f * f), r.getX(), r.getY(),
                                   juce::Colour (0xff575757).darker (0.08f * f), r.getRight(), r.getBottom(), false);
        g.setGradientFill (fill);
        g.fillEllipse (r);
        g.setColour (juce::Colour (0xff2c2c2c));
        g.drawEllipse (r, 1.4f);
    }

    const auto cap = ring.reduced (29.0f).translated (0.0f, 9.0f);
    juce::ColourGradient capFill (juce::Colour (0xffb9b9b5), cap.getX(), cap.getY(),
                                  juce::Colour (0xff828282), cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (capFill);
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0xff444444));
    g.drawEllipse (cap, 2.0f);
}

void drawClipReadOverlay (juce::Graphics& g, bool active)
{
    if (! active)
        return;

    const auto lamp = juce::Rectangle<float> (174.0f, 191.0f, 19.0f, 19.0f);
    juce::ColourGradient glow (juce::Colour (0xffff7a62).withAlpha (0.62f), lamp.getCentreX(), lamp.getCentreY(),
                               juce::Colours::transparentBlack, lamp.getCentreX(), lamp.getCentreY() + 24.0f, true);
    g.setGradientFill (glow);
    g.fillEllipse (lamp.expanded (18.0f));
    g.setColour (juce::Colour (0xffff2a20));
    g.fillEllipse (lamp.reduced (2.0f));
    g.setColour (juce::Colour (0xffff1e1e));
    g.fillRect (169, 234, 15, 23);
    g.setColour (juce::Colour (0xffff7267));
    g.fillRect (169, 266, 15, 13);
}

void drawStateOverlays (juce::Graphics& g,
                        juce::AudioProcessorValueTreeState& apvts,
                        bool clipActive,
                        bool advancedExpanded)
{
    const auto dark = isParamOn (apvts, ParameterIDs::darkMode);
    const auto postGate = getChoiceIndex (apvts, ParameterIDs::gatePrePost) == 1;
    const auto sendConnected = isParamOn (apvts, ParameterIDs::sendConnected);

    if (dark)
        drawDarkPressedOverlay (g);

    if (! postGate)
        drawGatePositionOverlay (g, false);

    if (sendConnected)
        drawFootswitchPressedOverlay (g);

    drawClipReadOverlay (g, clipActive);

    if (advancedExpanded)
        drawAdvancedFanout (g, juce::Colour (0xff5fc0d2));
}

} // namespace

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      clipLed ([this] { return processorRef.isClipHoldActive(); }),
      pressurePad (p.getAPVTS(), ParameterIDs::sendConnected, ParameterIDs::sendAmount),
      advancedDrawer (p.getAPVTS(),
                      ParameterIDs::inputThreshold,
                      ParameterIDs::sendFeel,
                      ParameterIDs::authenticColor,
                      ParameterIDs::extendedStereo,
                      ParameterIDs::dirtOs)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setVisible (false);
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < FactoryPresets::kNumPresets; ++i)
        presetBox.addItem (upperPresetName (i), i + 1);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { presetChanged(); };
    presetBox.setAlpha (0.0f);
    addAndMakeVisible (presetBox);

    for (auto* button : { &saveButton, &newButton, &deleteButton })
    {
        button->setAlpha (0.01f);
        addAndMakeVisible (*button);
    }

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
    {
        knob->setLabelColour (lookAndFeel.labelColour());
        knob->setAlpha (0.0f);
        addAndMakeVisible (*knob);
    }

    lvlKnob.setRangeText ("0", "10");
    lvlKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });
    sizeKnob.setRangeText ("1", "10");
    sizeKnob.setValueFormatter ([] (double value) { return juce::String (1.0 + value * 0.2, 2); });
    distnKnob.setRangeText ("0", "100");
    distnKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 2); });
    inKnob.setValueFormatter ([] (double value) { return formatSignedDbFromNorm (value); });
    outKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });

    addAndMakeVisible (darkToggle);
    addAndMakeVisible (gateToggle);
    addAndMakeVisible (clipLed);
    addAndMakeVisible (pressurePad);
    darkToggle.setAlpha (0.0f);
    gateToggle.setAlpha (0.0f);
    clipLed.setAlpha (0.0f);
    pressurePad.setAlpha (0.0f);

    advancedButton.setButtonText ("ADVANCED >");
    advancedButton.onClick = [this] { toggleAdvanced(); };
    advancedButton.setAlpha (0.0f);
    addAndMakeVisible (advancedButton);
    addChildComponent (advancedDrawer);

    auto& apvts = processorRef.getAPVTS();

    inAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::inputGain, inKnob.getSlider());
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::size, sizeKnob.getSlider());
    lvlAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::level, lvlKnob.getSlider());
    distnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::distn, distnKnob.getSlider());
    outAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::outputGain, outKnob.getSlider());
    darkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::darkMode, darkToggle);

    const auto gateIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::gatePrePost)->load());
    gateToggle.setToggleState (gateIndex == 1, juce::dontSendNotification);
    gateToggle.setButtonText ("Gate");
    gateToggle.onClick = [this, &apvts]
    {
        const auto post = gateToggle.getToggleState();
        gateToggle.setButtonText ("Gate");
        if (auto* param = apvts.getParameter (ParameterIDs::gatePrePost))
            param->setValueNotifyingHost (post ? 1.0f : 0.0f);
    };

    darkToggle.setButtonText ("Dark");

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    const auto cyan = lookAndFeel.cyanColour();

    auto faceplate = juce::ImageFileFormat::loadFrom (
        juce::File::getCurrentWorkingDirectory().getChildFile ("resources/ui/reverbx-faceplate.png"));
    if (! faceplate.isValid())
        faceplate = juce::ImageFileFormat::loadFrom (BinaryData::reverbxfaceplate_png,
                                                     static_cast<size_t> (BinaryData::reverbxfaceplate_pngSize));

    if (faceplate.isValid())
    {
        g.drawImage (faceplate, bounds);
        drawStateOverlays (g, processorRef.getAPVTS(), processorRef.isClipHoldActive(), advancedDrawer.isExpanded());
        return;
    }

    g.fillAll (juce::Colour (0xff0a0a0a));

    juce::ColourGradient chassis (juce::Colours::white, bounds.getX(), bounds.getY(),
                                  juce::Colour (0xffeeeeeb), bounds.getRight(), bounds.getBottom(), false);
    g.setGradientFill (chassis);
    g.fillRoundedRectangle (bounds.reduced (6.0f), 14.0f);
    g.setColour (juce::Colour (0xff253035));
    g.drawRoundedRectangle (bounds.reduced (6.0f), 14.0f, 2.0f);
    g.setColour (juce::Colours::white.withAlpha (0.5f));
    g.drawRoundedRectangle (bounds.reduced (9.0f), 11.0f, 2.0f);

    drawLogo (g, { 60.0f, 24.0f, 300.0f, 54.0f });

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (34.0f, juce::Font::bold));
    g.drawFittedText ("REVERB X", 104, 90, 218, 42, juce::Justification::centred, 1, 0.86f);
    g.setColour (cyan);
    g.drawLine (48.0f, 113.0f, 112.0f, 113.0f, 5.0f);
    g.drawLine (312.0f, 113.0f, 372.0f, 113.0f, 5.0f);

    drawCyanFrame (g, { 40.0f, 128.0f, 340.0f, 600.0f }, cyan);
    drawChevronRail (g, 24.0f, 270.0f, 4, cyan);
    drawChevronRail (g, 24.0f, 610.0f, 6, cyan);
    drawControlIcons (g, { 300, 148, 92, 44 });

    const auto leftPanel = juce::Rectangle<float> (58.0f, 195.0f, 154.0f, 219.0f);
    g.setColour (cyan);
    g.drawRect (leftPanel, 4.0f);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("OVERLOAD / CLIP", leftPanel.withHeight (42.0f).toNearestInt(), juce::Justification::centred);

    constexpr const char* reverb = "REVERB";
    g.setFont (juce::FontOptions (40.0f, juce::Font::bold));
    for (int i = 0; i < 6; ++i)
        g.drawText (juce::String::charToString (reverb[i]), 58, 248 + i * 25, 34, 28, juce::Justification::centred);

    g.setFont (juce::FontOptions (116.0f, juce::Font::bold));
    g.drawFittedText ("X", 98, 240, 96, 120, juce::Justification::centred, 1, 0.8f);
    g.setColour (cyan);
    for (int i = 0; i < 4; ++i)
    {
        const auto f = static_cast<float> (i);
        g.drawEllipse (138.0f + f * 8.0f, 258.0f + f * 7.0f, 56.0f + f * 12.0f, 78.0f - f * 4.0f, 2.0f);
    }

    g.setColour (cyan);
    g.fillRect (58, 415, 92, 26);
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("OVERLOAD", 61, 415, 86, 26, juce::Justification::centred);
    juce::Path redArrow;
    redArrow.addTriangle (158.0f, 413.0f, 195.0f, 428.0f, 158.0f, 443.0f);
    g.setColour (juce::Colour (0xffa52829));
    g.fillPath (redArrow);
    g.setColour (cyan);
    g.strokePath (redArrow, juce::PathStrokeType (3.0f));

    const auto diagonal = juce::Rectangle<float> (58.0f, 442.0f, 154.0f, 128.0f);
    g.setColour (cyan);
    g.drawRect (diagonal, 4.0f);
    {
        juce::Graphics::ScopedSaveState clipped (g);
        g.reduceClipRegion (diagonal.toNearestInt());
        for (int i = -2; i < 5; ++i)
        {
            const auto f = static_cast<float> (i);
            g.drawLine (diagonal.getX() + f * 36.0f, diagonal.getBottom(),
                        diagonal.getX() + 64.0f + f * 36.0f, diagonal.getY(), 4.0f);
        }
    }
    fillTriangle (g, 94.0f, 522.0f, 112.0f, 516.0f, 105.0f, 536.0f);
    fillTriangle (g, 112.0f, 508.0f, 127.0f, 502.0f, 121.0f, 518.0f);
    fillTriangle (g, 148.0f, 470.0f, 159.0f, 482.0f, 140.0f, 486.0f);
    fillTriangle (g, 166.0f, 452.0f, 179.0f, 464.0f, 158.0f, 468.0f);

    const auto green = juce::Rectangle<float> (94.0f, 486.0f, 58.0f, 58.0f);
    g.setColour (juce::Colour (0xff295a34));
    g.fillEllipse (green.expanded (6.0f));
    juce::ColourGradient greenFill (juce::Colour (0xff98ff56), green.getX(), green.getY(),
                                    juce::Colour (0xff2f9b39), green.getRight(), green.getBottom(), false);
    g.setGradientFill (greenFill);
    g.fillEllipse (green);

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (14.0f, juce::Font::bold));
    g.drawText ("DARK MODE", 58, 574, 96, 22, juce::Justification::centredLeft);
    g.drawText ("GATE", 156, 574, 56, 22, juce::Justification::centred);
    g.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    g.drawText ("PRE", 204, 594, 44, 24, juce::Justification::centredLeft);
    g.drawText ("POST", 204, 620, 50, 24, juce::Justification::centredLeft);
    fillTriangle (g, 213.0f, 566.0f, 222.0f, 558.0f, 231.0f, 566.0f);
    fillTriangle (g, 213.0f, 652.0f, 222.0f, 660.0f, 231.0f, 652.0f);

    const auto bottomBlock = juce::Rectangle<float> (48.0f, 650.0f, 198.0f, 96.0f);
    g.setColour (juce::Colour (0xff111111));
    g.fillRoundedRectangle (bottomBlock, 6.0f);
    g.setColour (cyan);
    g.fillRect (bottomBlock.withTrimmedTop (50.0f));

    juce::Path advancedClosed;
    advancedClosed.startNewSubPath (258.0f, 646.0f);
    advancedClosed.lineTo (382.0f, 646.0f);
    advancedClosed.lineTo (382.0f, 726.0f);
    advancedClosed.lineTo (356.0f, 750.0f);
    advancedClosed.lineTo (250.0f, 750.0f);
    advancedClosed.lineTo (250.0f, 664.0f);
    advancedClosed.closeSubPath();
    g.setColour (juce::Colours::black);
    g.fillPath (advancedClosed);
    g.setColour (cyan);
    g.strokePath (advancedClosed, juce::PathStrokeType (4.0f));

    if (advancedDrawer.isExpanded())
        drawAdvancedFanout (g, cyan);
}

void PluginEditor::resized()
{
    titleLabel.setBounds (0, 0, 0, 0);
    presetBox.setBounds (64, 148, 234, 30);
    saveButton.setBounds (300, 148, 30, 44);
    newButton.setBounds (330, 148, 30, 44);
    deleteButton.setBounds (360, 148, 36, 44);

    clipLed.setBounds (184, 208, 42, 206);

    lvlKnob.setBounds (228, 196, 184, 90);
    sizeKnob.setBounds (228, 284, 184, 90);
    distnKnob.setBounds (228, 374, 188, 92);
    inKnob.setBounds (228, 468, 188, 90);
    outKnob.setBounds (228, 560, 188, 90);

    darkToggle.setBounds (70, 604, 54, 54);
    gateToggle.setBounds (166, 598, 34, 70);
    pressurePad.setBounds (48, 650, 198, 96);

    advancedButton.setBounds (258, 656, 106, 34);
    advancedDrawer.setBounds (getAdvancedBounds());
}

void PluginEditor::presetChanged()
{
    const auto index = presetBox.getSelectedId() - 1;
    if (index >= 0)
        processorRef.setCurrentProgram (index);
}

void PluginEditor::toggleAdvanced()
{
    setAdvancedExpandedForSnapshot (! advancedDrawer.isExpanded());
}

juce::Rectangle<int> PluginEditor::getAdvancedBounds() const
{
    return { 232, 572, 166, advancedDrawer.getPreferredHeight() };
}

void PluginEditor::setAdvancedExpandedForSnapshot (bool shouldExpand)
{
    advancedDrawer.setExpanded (shouldExpand);
    advancedButton.setButtonText (shouldExpand ? "ADVANCED <" : "ADVANCED >");
    advancedButton.setVisible (! shouldExpand);
    resized();
    if (shouldExpand)
        advancedDrawer.toFront (false);
    repaint();
}

void PluginEditor::timerCallback()
{
    repaint();
}

} // namespace sendbloom
