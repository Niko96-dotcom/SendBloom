#include "AdvancedDrawer.h"
#include "ParameterCurves.h"

namespace sendbloom::ui
{

AdvancedDrawer::AdvancedDrawer (juce::AudioProcessorValueTreeState& apvts,
                                const juce::String& thresholdId,
                                const juce::String& sendFeelId,
                                const juce::String& extendedStereoId,
                                const juce::String& sendConnectedId)
{
    gateSensKnob.setLabelBackgroundVisible (false);
    pressureModeToggle.setLookAndFeel (&serviceLookAndFeel);
    extendedStereoToggle.setLookAndFeel (&serviceLookAndFeel);
    addChildComponent (gateSensKnob);
    gateSensKnob.setLabelColour (juce::Colour (0xff292d2e));
    gateSensKnob.setDefaultValue (0.5);
    gateSensKnob.setValueFormatter ([] (double value)
    {
        // CORE-07: Gate Sens reports canonical threshold dB.
        return juce::String (ParameterCurves::inputThresholdDb (static_cast<float> (value)), 1) + " dB";
    });

    sendFeelLabel.setText ("SEND FEEL", juce::dontSendNotification);
    sendFeelLabel.setJustificationType (juce::Justification::centred);
    sendFeelLabel.setColour (juce::Label::textColourId, juce::Colour (0xff292d2e));
    sendFeelLabel.setFont (juce::FontOptions (13.0f, juce::Font::bold));
    addChildComponent (sendFeelLabel);

    sendFeelBox.addItem ("Firm", 1);
    sendFeelBox.addItem ("Soft", 2);
    addChildComponent (sendFeelBox);

    pressureModeToggle.setTooltip ("Pressure Mode: when on, wet feed follows pressure; "
                                   "when off, reverb stays always-on.");
    pressureModeToggle.setComponentID ("advanced-toggle-pressure");
    addChildComponent (pressureModeToggle);

    addChildComponent (extendedStereoToggle);
    extendedStereoToggle.setComponentID ("advanced-toggle-stereo");

    extendedStereoToggle.setTooltip ("Preserve the original left/right dry image while sharing "
                                     "the mono wet return across both channels.");

    gateSensAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, thresholdId, gateSensKnob.getSlider());
    sendFeelAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (
        apvts, sendFeelId, sendFeelBox);
    pressureModeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, sendConnectedId, pressureModeToggle);
    extendedStereoAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, extendedStereoId, extendedStereoToggle);
}

AdvancedDrawer::~AdvancedDrawer()
{
    pressureModeToggle.setLookAndFeel(nullptr);
    extendedStereoToggle.setLookAndFeel(nullptr);
}

void AdvancedDrawer::paint (juce::Graphics& g)
{
    auto tray = getLocalBounds().toFloat().reduced (1.0f);
    g.setColour (juce::Colours::black.withAlpha (0.23f));
    g.fillRoundedRectangle (tray.translated (0.0f, 3.0f), 5.0f);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffeeecdf), tray.getTopLeft(),
                                            juce::Colour (0xffd8d8c8), tray.getBottomRight(), false));
    g.fillRoundedRectangle (tray, 5.0f);
    g.setColour (juce::Colour (0xff777d70));
    g.drawRoundedRectangle (tray, 5.0f, 1.0f);
    g.setColour (juce::Colours::white.withAlpha (0.9f));
    g.drawRoundedRectangle (tray.reduced (2.0f), 3.0f, 1.0f);
    g.setColour (niko::clear::palette::ink);
    g.setFont (niko::clear::sans (12.0f, true));
    g.drawText ("SEND SETTINGS", 154, 16, 190, 22, juce::Justification::centredLeft);
}

void AdvancedDrawer::setExpanded (bool shouldExpand)
{
    expanded = shouldExpand;
    gateSensKnob.setVisible (expanded);
    sendFeelLabel.setVisible (expanded);
    sendFeelBox.setVisible (expanded);
    pressureModeToggle.setVisible (expanded);
    extendedStereoToggle.setVisible (expanded);
    setVisible (expanded);
    resized();
}

void AdvancedDrawer::resized()
{
    if (! expanded)
        return;

    gateSensKnob.setBounds (26, 10, 104, 132);
    sendFeelLabel.setBounds (154, 49, 126, 20);
    sendFeelBox.setBounds (154, 77, 126, 34);
    pressureModeToggle.setBounds (320, 30, 176, 36);
    extendedStereoToggle.setBounds (320, 83, 176, 36);

}

} // namespace sendbloom::ui
