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

class PluginEditor : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void presetChanged();
    void toggleAdvanced();

    PluginProcessor& processorRef;
    ui::SendBloomLookAndFeel lookAndFeel;

    juce::Label titleLabel;
    juce::ComboBox presetBox;
    ui::PedalKnob inKnob { "In" };
    ui::PedalKnob sizeKnob { "Size" };
    ui::PedalKnob lvlKnob { "Lvl" };
    ui::PedalKnob distnKnob { "Distn" };
    ui::PedalKnob outKnob { "Out" };
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
