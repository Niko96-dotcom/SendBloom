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
    presetBox.setLookAndFeel (&transparentControls);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::arrowColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (presetBox);

    for (auto* button : { &saveButton, &newButton, &deleteButton })
    {
        button->setLookAndFeel (&transparentControls);
        button->setColour (juce::TextButton::buttonColourId, juce::Colours::transparentBlack);
        button->setColour (juce::TextButton::buttonOnColourId, juce::Colours::transparentBlack);
        button->setColour (juce::TextButton::textColourOffId, juce::Colours::transparentBlack);
        button->setColour (juce::TextButton::textColourOnId, juce::Colours::transparentBlack);
        addAndMakeVisible (*button);
    }

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
        addAndMakeVisible (*knob);

    lvlKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });
    sizeKnob.setValueFormatter ([] (double value) { return juce::String (1.0 + value * 0.2, 2); });
    distnKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 2); });
    inKnob.setValueFormatter ([] (double value) { return formatSignedDbFromNorm (value); });
    outKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });

    darkToggle.setLookAndFeel (&transparentControls);
    gateToggle.setLookAndFeel (&transparentControls);
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

    const auto gateIndex = static_cast<int> (apvts.getRawParameterValue (ParameterIDs::gatePrePost)->load());
    gateToggle.setToggleState (gateIndex == 1, juce::dontSendNotification);
    gateToggle.setButtonText ("Gate");
    gateToggle.onClick = [this, &apvts]
    {
        const auto post = gateToggle.getToggleState();
        if (auto* param = apvts.getParameter (ParameterIDs::gatePrePost))
            param->setValueNotifyingHost (post ? 1.0f : 0.0f);
        repaint();
    };

    darkToggle.onStateChange = [this] { repaint(); };

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    presetBox.setLookAndFeel (nullptr);
    saveButton.setLookAndFeel (nullptr);
    newButton.setLookAndFeel (nullptr);
    deleteButton.setLookAndFeel (nullptr);
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
                             advancedDrawer.isExpanded());

    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawFittedText (presetBox.getText(),
                      { 72, 150, 200, 26 },
                      juce::Justification::centredLeft,
                      1);
}

void PluginEditor::resized()
{
    titleLabel.setBounds (0, 0, 0, 0);
    presetBox.setBounds (64, 148, 234, 30);
    saveButton.setBounds (300, 148, 30, 44);
    newButton.setBounds (330, 148, 30, 44);
    deleteButton.setBounds (360, 148, 36, 44);

    // Cropped knob art centres measured from cyan rings on the faceplate asset.
    lvlKnob.setBounds (265 - 32, 213 - 32, 64, 84);
    sizeKnob.setBounds (265 - 32, 302 - 32, 64, 84);
    distnKnob.setBounds (265 - 32, 392 - 32, 64, 84);
    inKnob.setBounds (265 - 32, 473 - 32, 64, 84);
    outKnob.setBounds (265 - 32, 556 - 32, 64, 84);

    darkToggle.setBounds (64, 568, 50, 50);
    gateToggle.setBounds (143, 560, 27, 64);
    pressurePad.setBounds (90, 650, 95, 95);

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
