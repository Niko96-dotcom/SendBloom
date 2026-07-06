#include "PluginEditor.h"
#include "FactoryPresets.h"

namespace sendbloom
{

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

    titleLabel.setText ("SendBloom", juce::dontSendNotification);
    titleLabel.setJustificationType (juce::Justification::centred);
    titleLabel.setFont (juce::FontOptions (16.0f, juce::Font::bold));
    titleLabel.setColour (juce::Label::textColourId, lookAndFeel.labelColour());
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < FactoryPresets::kNumPresets; ++i)
        presetBox.addItem (FactoryPresets::getPresetName (i), i + 1);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { presetChanged(); };
    addAndMakeVisible (presetBox);

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
    {
        knob->setLabelColour (lookAndFeel.labelColour());
        addAndMakeVisible (*knob);
    }

    addAndMakeVisible (darkToggle);
    addAndMakeVisible (gateToggle);
    addAndMakeVisible (clipLed);
    addAndMakeVisible (pressurePad);

    advancedButton.onClick = [this] { toggleAdvanced(); };
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
    gateToggle.setButtonText (gateIndex == 1 ? "Gate Post" : "Gate Pre");
    gateToggle.onClick = [this, &apvts]
    {
        const auto post = gateToggle.getToggleState();
        gateToggle.setButtonText (post ? "Gate Post" : "Gate Pre");
        if (auto* param = apvts.getParameter (ParameterIDs::gatePrePost))
            param->setValueNotifyingHost (post ? 1.0f : 0.0f);
    };

    setSize (340, 520);
}

PluginEditor::~PluginEditor()
{
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();
    g.setColour (lookAndFeel.chassisColour());
    g.fillRoundedRectangle (bounds, 12.0f);
    g.setColour (lookAndFeel.facePlateColour().brighter (0.1f));
    g.drawRoundedRectangle (bounds.reduced (1.0f), 12.0f, 1.5f);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds().reduced (16);
    titleLabel.setBounds (area.removeFromTop (24));
    presetBox.setBounds (area.removeFromTop (24).reduced (20, 0));
    area.removeFromTop (8);

    auto knobRow = area.removeFromTop (72);
    const auto knobWidth = knobRow.getWidth() / 5;
    inKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    sizeKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    lvlKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    distnKnob.setBounds (knobRow.removeFromLeft (knobWidth));
    outKnob.setBounds (knobRow);

    area.removeFromTop (8);
    auto toggleRow = area.removeFromTop (24);
    darkToggle.setBounds (toggleRow.removeFromLeft (80));
    gateToggle.setBounds (toggleRow.removeFromLeft (100));
    clipLed.setBounds (toggleRow.removeFromRight (48).withHeight (36));

    area.removeFromTop (8);
    pressurePad.setBounds (area.removeFromTop (100).reduced (40, 0));

    area.removeFromTop (8);
    advancedButton.setBounds (area.removeFromTop (24).reduced (80, 0));

    if (advancedDrawer.isExpanded())
    {
        area.removeFromTop (4);
        advancedDrawer.setBounds (area.removeFromTop (advancedDrawer.getPreferredHeight()));
    }
}

void PluginEditor::presetChanged()
{
    const auto index = presetBox.getSelectedId() - 1;
    if (index >= 0)
        processorRef.setCurrentProgram (index);
}

void PluginEditor::toggleAdvanced()
{
    const auto expand = ! advancedDrawer.isExpanded();
    advancedDrawer.setExpanded (expand);
    advancedButton.setButtonText (expand ? "Advanced \u25b4" : "Advanced \u25be");
    resized();
}

} // namespace sendbloom
