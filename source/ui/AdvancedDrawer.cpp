#include "AdvancedDrawer.h"

namespace sendbloom::ui
{

AdvancedDrawer::AdvancedDrawer (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& thresholdId,
                                const juce::String& sendFeelId,
                                const juce::String& authenticColorId,
                                const juce::String& extendedStereoId,
                                const juce::String& dirtOsId,
                                const juce::String& sendConnectedId)
{
    addChildComponent (gateSensKnob);
    gateSensKnob.setLabelColour (juce::Colour (0xff5fc0d2));
    gateSensKnob.setRangeText ("", "");
    gateSensKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });

    sendFeelLabel.setText ("SEND FEEL", juce::dontSendNotification);
    sendFeelLabel.setJustificationType (juce::Justification::centred);
    sendFeelLabel.setColour (juce::Label::textColourId, juce::Colour (0xff5fc0d2));
    sendFeelLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addChildComponent (sendFeelLabel);

    sendFeelBox.addItem ("Firm", 1);
    sendFeelBox.addItem ("Soft", 2);
    addChildComponent (sendFeelBox);

    pressureModeToggle.setTooltip ("Pressure Mode: when on, wet feed follows pressure; "
                                   "when off, reverb stays always-on.");
    addChildComponent (pressureModeToggle);

    colorToggle.setTooltip ("Experimental — off by default until validated. "
                            "Steps the tank at 32,768 Hz with fixed delay-table lengths, "
                            "per-comb feedback, damping, and 9-bit quantization. "
                            "Original software — not firmware-derived. "
                            "May exhibit HF artifacts at some host rates; "
                            "host-rate reverb is the production default.");
    addChildComponent (colorToggle);
    addChildComponent (extendedStereoToggle);
    addChildComponent (dirtOsToggle);

    extendedStereoToggle.setEnabled (false);
    dirtOsToggle.setEnabled (false);
    extendedStereoToggle.setTooltip ("Coming soon");
    dirtOsToggle.setTooltip ("Coming soon");

    gateSensAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, thresholdId, gateSensKnob.getSlider());
    sendFeelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, sendFeelId, sendFeelBox);
    pressureModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, sendConnectedId, pressureModeToggle);
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, authenticColorId, colorToggle);
    extendedStereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, extendedStereoId, extendedStereoToggle);
    dirtOsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, dirtOsId, dirtOsToggle);
}

void AdvancedDrawer::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat();

    juce::Path panel;
    panel.startNewSubPath (14.0f, 0.0f);
    panel.lineTo (bounds.getRight() - 3.0f, 0.0f);
    panel.lineTo (bounds.getRight() - 3.0f, bounds.getBottom() - 28.0f);
    panel.lineTo (bounds.getRight() - 28.0f, bounds.getBottom() - 3.0f);
    panel.lineTo (2.0f, bounds.getBottom() - 3.0f);
    panel.lineTo (2.0f, 14.0f);
    panel.closeSubPath();

    g.setColour (juce::Colours::black);
    g.fillPath (panel);
    g.setColour (juce::Colour (0xff5fc0d2));
    g.strokePath (panel, juce::PathStrokeType (4.0f));

    g.setColour (juce::Colour (0xff5fc0d2));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("ADVANCED", 18, 8, getWidth() - 36, 24, juce::Justification::centred);
}

void AdvancedDrawer::setExpanded (bool shouldExpand)
{
    expanded = shouldExpand;
    gateSensKnob.setVisible (expanded);
    sendFeelLabel.setVisible (expanded);
    sendFeelBox.setVisible (expanded);
    pressureModeToggle.setVisible (expanded);
    colorToggle.setVisible (expanded);
    extendedStereoToggle.setVisible (expanded);
    dirtOsToggle.setVisible (expanded);
    setVisible (expanded);
    resized();
}

void AdvancedDrawer::resized()
{
    if (! expanded)
        return;

    auto area = getLocalBounds().reduced (14);
    area.removeFromTop (30);
    auto topRow = area.removeFromTop (88);
    gateSensKnob.setBounds (topRow.removeFromLeft (88));

    auto feelArea = topRow.removeFromLeft (98).reduced (4, 10);
    sendFeelLabel.setBounds (feelArea.removeFromTop (16));
    sendFeelBox.setBounds (feelArea.removeFromTop (28));

    colorToggle.setBounds (topRow.removeFromLeft (98).reduced (8, 24));
    pressureModeToggle.setBounds (area.removeFromTop (26).reduced (8, 0));
    extendedStereoToggle.setBounds (area.removeFromTop (26).reduced (8, 0));
    dirtOsToggle.setBounds (area.removeFromTop (26).reduced (8, 0));
}

} // namespace sendbloom::ui
