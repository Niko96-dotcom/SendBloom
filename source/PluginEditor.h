#pragma once

#include "PluginProcessor.h"
#include "ui/NikoClearLookAndFeel.h"
#include "ui/SendBloomSceneArt.h"
#include "ui/SendBloomSceneLookAndFeel.h"
#include "ui/PedalKnob.h"
#include "ui/PressureSendPad.h"
#include "ui/AdvancedDrawer.h"
#include "ParameterIDs.h"

namespace sendbloom
{

class PluginEditor : public juce::AudioProcessorEditor,
                     private juce::Timer
{
public:
    enum class PresetActionSnapshotState
    {
        none,
        loadHover,
        loadFocus,
        saveHover,
        saveFocus,
        loadDown,
        saveDown,
        advancedDown,
    };

    explicit PluginEditor (PluginProcessor&);
    ~PluginEditor() override;

    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;
    void setAdvancedExpandedForSnapshot (bool shouldExpand);
    void setPresetActionStateForSnapshot (PresetActionSnapshotState state);
    void paintPresetMenuForSnapshot (juce::Graphics& g);

private:
    void presetChanged();
    void toggleAdvanced();
    juce::Rectangle<int> getAdvancedBounds() const;
    void timerCallback() override;

    PluginProcessor& processorRef;
    niko::clear::LookAndFeel lookAndFeel;
    const ui::SendBloomSceneArt& sceneArt = ui::SendBloomSceneArt::instance();
    ui::SendBloomSceneLookAndFeel sceneLookAndFeel;

    juce::ComboBox presetBox;
    ui::PedalKnob inKnob { "INPUT" };
    ui::PedalKnob sizeKnob { "SIZE" };
    ui::PedalKnob lvlKnob { "LEVEL" };
    ui::PedalKnob distnKnob { "DISTORTION" };
    ui::PedalKnob outKnob { "OUTPUT" };
    juce::ToggleButton darkToggle { "Dark" };
    juce::ToggleButton gateToggle { "GATE POST" };
    juce::ToggleButton bypassToggle { "BYPASS" };
    ui::PressureSendPad pressurePad;
    juce::TextButton advancedButton { "Advanced" };
    juce::TextButton loadPresetButton;
    juce::TextButton savePresetButton;
    ui::AdvancedDrawer advancedDrawer;
    std::unique_ptr<juce::FileChooser> presetFileChooser;
    PresetActionSnapshotState presetActionSnapshotState { PresetActionSnapshotState::none };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> inAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> sizeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> lvlAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> distnAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> outAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> darkAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> gateAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    juce::TooltipWindow tooltipWindow { this, 650 };

    void loadPresetFromDisk();
    void savePresetToDisk();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace sendbloom
