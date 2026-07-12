#include "PluginEditor.h"
#include "FactoryPresets.h"
#include "ParameterCurves.h"
#include "ui/PedalFaceplatePaint.h"

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

juce::String formatInputGainDb (double norm)
{
    // CORE-02 / ADR-V1-08: display must call the canonical DSP curve.
    const auto db = ParameterCurves::inputGainDb (static_cast<float> (norm));
    if (std::abs (db) < 0.005f)
        return "0.00";

    return juce::String (db, 2);
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
                      ParameterIDs::sendConnected)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setVisible (false);
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < FactoryPresets::kNumPresets; ++i)
        presetBox.addItem (upperPresetName (i), i + 1);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        presetChanged();
        repaint();
    };
    presetBox.setLookAndFeel (&transparentControls);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::arrowColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (presetBox);
    // Kill the ComboBox's internal label — faceplate owns the default glyphs.
    for (auto* child : presetBox.getChildren())
        if (auto* label = dynamic_cast<juce::Label*> (child))
            label->setAlpha (0.0f);

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
        addAndMakeVisible (*knob);

    lvlKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });
    sizeKnob.setValueFormatter ([] (double value) { return juce::String (1.0 + value * 0.2, 2); });
    distnKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 2); });
    inKnob.setValueFormatter ([] (double value) { return formatInputGainDb (value); });
    outKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });

    darkToggle.setLookAndFeel (&transparentControls);
    gateToggle.setLookAndFeel (&transparentControls);
    darkToggle.setClickingTogglesState (true);
    gateToggle.setClickingTogglesState (true);
    addAndMakeVisible (darkToggle);
    addAndMakeVisible (gateToggle);

    clipLed.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (clipLed);
    clipLed.setVisible (false);

    pressurePad.setOpaque (false);
    addAndMakeVisible (pressurePad);

    advancedButton.setButtonText ("ADVANCED >");
    advancedButton.onClick = [this] { toggleAdvanced(); };
    advancedButton.setLookAndFeel (&transparentControls);
    advancedButton.setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
    advancedButton.setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
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
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::gatePrePost, gateToggle);
    gateToggle.setButtonText ("Gate");
    gateToggle.onStateChange = [this] { repaint(); };

    darkToggle.onStateChange = [this] { repaint(); };

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    presetBox.setLookAndFeel (nullptr);
    darkToggle.setLookAndFeel (nullptr);
    gateToggle.setLookAndFeel (nullptr);
    advancedButton.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    ui::paintPedalFaceplate (g,
                             getLocalBounds().toFloat(),
                             lookAndFeel.cyanColour(),
                             processorRef.getAPVTS(),
                             processorRef.isClipHoldActive(),
                             advancedDrawer.isExpanded(),
                             pressurePad.isPressed(),
                             pressurePad.getDisplayAmount());

    // The procedural chassis paints the default "INITIAL PATCH" label; redraw only overrides.
    if (presetBox.getSelectedId() != 1)
    {
        g.setColour (juce::Colours::white);
        g.fillRect (70, 148, 200, 14);
        g.setColour (juce::Colours::black);
        g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
        g.drawText (presetBox.getText(),
                    76, 148, 184, 10,
                    juce::Justification::centredLeft,
                    false);
    }
}

void PluginEditor::resized()
{
    titleLabel.setBounds (0, 0, 0, 0);
    // Invisible hit target over the faceplate preset field (name is painted by the editor).
    presetBox.setBounds (64, 148, 210, 16);

    // Exact dark-disk centres measured from the faceplate asset (50×50 art).
    constexpr int k = 50;
    constexpr int half = k / 2;

    lvlKnob.setBounds (265 - half, 213 - half, k, 76);
    sizeKnob.setBounds (265 - half, 303 - half, k, 76);
    distnKnob.setBounds (265 - half, 393 - half, k, 76); // dark-disk centre
    inKnob.setBounds (265 - half, 479 - half, k, 76);
    outKnob.setBounds (265 - half, 564 - half, k, 76);

    lvlKnob.setValueBounds ({ 0, 60, 51, 15 });
    sizeKnob.setValueBounds ({ 0, 60, 51, 15 });
    distnKnob.setValueBounds ({ -1, 60, 52, 15 });
    inKnob.setValueBounds ({ -1, 58, 53, 15 });
    outKnob.setValueBounds ({ -1, 58, 53, 15 });

    // Dark button on faceplate: ~64,562 .. 103,598
    darkToggle.setBounds (64, 562, 40, 40);
    gateToggle.setBounds (148, 568, 22, 52);
    pressurePad.setBounds (90, 655, 95, 90);

    advancedButton.setBounds (250, 646, 132, 104);
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
    return { 232, 560, 166, advancedDrawer.getPreferredHeight() };
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
