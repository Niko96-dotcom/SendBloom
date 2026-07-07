#include "AdvancedDrawer.h"

namespace sendbloom::ui
{

AdvancedDrawer::AdvancedDrawer (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& thresholdId,
                                const juce::String& sendFeelId,
                                const juce::String& authenticColorId,
                                const juce::String& extendedStereoId,
                                const juce::String& dirtOsId)
{
    addChildComponent (gateSensKnob);
    gateSensKnob.setLabelColour (juce::Colour (0xffE8E6E3));

    sendFeelLabel.setText ("Send Feel", juce::dontSendNotification);
    sendFeelLabel.setJustificationType (juce::Justification::centred);
    sendFeelLabel.setColour (juce::Label::textColourId, juce::Colour (0xffE8E6E3));
    sendFeelLabel.setFont (juce::FontOptions (11.0f));
    addChildComponent (sendFeelLabel);

    sendFeelBox.addItem ("Firm", 1);
    sendFeelBox.addItem ("Soft", 2);
    addChildComponent (sendFeelBox);

    colorToggle.setTooltip ("Steps the tank at 32,768 Hz with fixed delay-table lengths, "
                            "per-comb feedback, damping, and 9-bit quantization. "
                            "Original software — not firmware-derived.");
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
    colorAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, authenticColorId, colorToggle);
    extendedStereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, extendedStereoId, extendedStereoToggle);
    dirtOsAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, dirtOsId, dirtOsToggle);
}

void AdvancedDrawer::setExpanded (bool shouldExpand)
{
    expanded = shouldExpand;
    gateSensKnob.setVisible (expanded);
    sendFeelLabel.setVisible (expanded);
    sendFeelBox.setVisible (expanded);
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

    auto area = getLocalBounds().reduced (8);
    auto topRow = area.removeFromTop (80);
    gateSensKnob.setBounds (topRow.removeFromLeft (72));

    auto feelArea = topRow.removeFromLeft (100);
    sendFeelLabel.setBounds (feelArea.removeFromTop (16));
    sendFeelBox.setBounds (feelArea.removeFromTop (24));

    colorToggle.setBounds (topRow.removeFromLeft (90).reduced (0, 24));
    extendedStereoToggle.setBounds (area.removeFromTop (24));
    dirtOsToggle.setBounds (area.removeFromTop (24));
}

} // namespace sendbloom::ui
