#include "PluginEditor.h"
#include "FactoryPresets.h"
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

juce::String formatSignedDbFromNorm (double value)
{
    const auto db = (0.5 - value) * 18.0;
    if (std::abs (db) < 0.005)
        return "-0.00";

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
    ui::paintPedalFaceplate (g,
                             getLocalBounds().toFloat(),
                             lookAndFeel.cyanColour(),
                             processorRef.getAPVTS(),
                             processorRef.isClipHoldActive(),
                             advancedDrawer.isExpanded());
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
