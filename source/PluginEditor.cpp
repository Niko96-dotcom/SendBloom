#include "PluginEditor.h"

namespace sendbloom
{

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    setSize (400, 300);
}

PluginEditor::~PluginEditor() = default;

void PluginEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (16.0f);
    const auto label = juce::String (PRODUCT_NAME_WITHOUT_VERSION) + " v" VERSION;
    g.drawFittedText (label, getLocalBounds(), juce::Justification::centred, 1);
}

void PluginEditor::resized()
{
}

} // namespace sendbloom
