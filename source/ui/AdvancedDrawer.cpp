#include "AdvancedDrawer.h"
#include "ParameterCurves.h"

namespace sendbloom::ui
{

AdvancedDrawer::AdvancedDrawer (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& thresholdId,
                                const juce::String& sendFeelId,
                                const juce::String& authenticColorId,
                                const juce::String& extendedStereoId,
                                const juce::String& sendConnectedId)
{
    addChildComponent (gateSensKnob);
    gateSensKnob.setLabelColour (juce::Colour (0xffe66c0b));
    gateSensKnob.setRangeText ("", "");
    gateSensKnob.setValueFormatter ([] (double value)
    {
        // CORE-07: Gate Sens reports canonical threshold dB.
        return juce::String (ParameterCurves::inputThresholdDb (static_cast<float> (value)), 2);
    });

    sendFeelLabel.setText ("SEND FEEL", juce::dontSendNotification);
    sendFeelLabel.setJustificationType (juce::Justification::centred);
    sendFeelLabel.setColour (juce::Label::textColourId, juce::Colour (0xffe66c0b));
    sendFeelLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addChildComponent (sendFeelLabel);

    sendFeelBox.addItem ("Firm", 1);
    sendFeelBox.addItem ("Soft", 2);
    const auto orange = juce::Colour (0xffe66c0b);
    sendFeelBox.setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff191411));
    sendFeelBox.setColour (juce::ComboBox::textColourId, orange);
    sendFeelBox.setColour (juce::ComboBox::outlineColourId, orange.withAlpha (0.55f));
    sendFeelBox.setColour (juce::ComboBox::arrowColourId, orange);
    sendFeelBox.setColour (juce::ComboBox::buttonColourId, juce::Colour (0xff191411));
    addChildComponent (sendFeelBox);

    pressureModeToggle.setTooltip ("Pressure Mode: when on, wet feed follows pressure; "
                                   "when off, reverb stays always-on.");
    addChildComponent (pressureModeToggle);

    colorToggle.setTooltip ("Experimental — off by default until validated. "
                            "Steps the tank at 32,768 Hz with fixed delay-table lengths, "
                            "per-comb feedback, damping, and 9-bit quantization. "
                            "Original software — not firmware-derived. "
                            "host-rate reverb is the production default.");
    addChildComponent (colorToggle);
    addChildComponent (extendedStereoToggle);

    extendedStereoToggle.setTooltip ("Preserve the original left/right dry image while sharing "
                                     "the mono wet return across both channels.");

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
    g.setColour (juce::Colour (0xffe66c0b));
    g.strokePath (panel, juce::PathStrokeType (4.0f));

    g.setColour (juce::Colour (0xffe66c0b));
    g.setFont (juce::FontOptions (18.0f, juce::Font::bold));
    g.drawText ("ADVANCED <", 18, 8, getWidth() - 36, 24, juce::Justification::centred);

    g.setFont (juce::FontOptions (9.0f, juce::Font::bold));
    g.drawText ("PRESSURE MODE", 14, 124, 108, 20, juce::Justification::centredLeft, false);
    g.drawText ("32K COLOR", 14, 150, 108, 20, juce::Justification::centredLeft, false);
    g.drawText ("EXTENDED STEREO", 14, 176, 108, 20, juce::Justification::centredLeft, false);
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
    setVisible (expanded);
    resized();
}

void AdvancedDrawer::resized()
{
    if (! expanded)
        return;

    gateSensKnob.setBounds (16, 38, 68, 82);
    sendFeelLabel.setBounds (86, 43, 66, 16);
    sendFeelBox.setBounds (84, 63, 68, 26);

    pressureModeToggle.setBounds (126, 122, 24, 22);
    colorToggle.setBounds (126, 148, 24, 22);
    extendedStereoToggle.setBounds (126, 174, 24, 22);
}

} // namespace sendbloom::ui
