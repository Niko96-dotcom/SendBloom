#pragma once

#include "PluginProcessor.h"
#include "ui/SendBloomLookAndFeel.h"
#include "ui/PedalKnob.h"
#include "ui/PressureSendPad.h"
#include "ui/ClipLed.h"
#include "ui/AdvancedDrawer.h"
#include "ParameterIDs.h"

namespace sendbloom
{

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void setAdvancedExpandedForSnapshot (bool shouldExpand);

private:
    void presetChanged();
    void toggleAdvanced();
    juce::Rectangle<int> getAdvancedBounds() const;
    void timerCallback() override;

    PluginProcessor& processorRef;
    ui::SendBloomLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::ComboBox presetBox;
    juce::TextButton saveButton { "SAVE" };
    juce::TextButton newButton { "NEW" };
    juce::TextButton deleteButton { "DELETE" };
    ui::PedalKnob inKnob { "INPUT (dB)" };
    ui::PedalKnob sizeKnob { "SIZE" };
    ui::PedalKnob lvlKnob { "LEVEL" };
    ui::PedalKnob distnKnob { "DISTORTION" };
    ui::PedalKnob outKnob { "OUTPUT (dB)" };
    juce::ToggleButton darkToggle { "Dark" };
    juce::ToggleButton gateToggle { "Gate Post" };
    ui::ClipLed clipLed;
    ui::PressureSendPad pressurePad;
    juce::TextButton advancedButton { "Advanced \u25be" };
    ui::AdvancedDrawer advancedDrawer;

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lvlAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> darkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace sendbloom
