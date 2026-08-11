#pragma once

#include "PluginProcessor.h"
#include "ui/SendBloomLookAndFeel.h"
#include "ui/TransparentControls.h"
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
    enum class PresetActionSnapshotState
    {
        none,
        loadHover,
        loadFocus,
        saveHover,
        saveFocus,
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
    ui::SendBloomLookAndFeel lookAndFeel;
    ui::TransparentControlsLookAndFeel transparentControls;

    juce::Label titleLabel;
    juce::ComboBox presetBox;
    // A matched hardware family: skirted main controls and straight-sided
    // mini controls, each path-traced one frame per pointer angle in the
    // faceplate's own light rig.
    ui::PedalKnob inKnob { "INPUT", BinaryData::knob_small_strip_png, BinaryData::knob_small_strip_pngSize };
    ui::PedalKnob sizeKnob { "SIZE", BinaryData::knob_large_strip_png, BinaryData::knob_large_strip_pngSize };
    ui::PedalKnob lvlKnob { "LEVEL", BinaryData::knob_large_strip_png, BinaryData::knob_large_strip_pngSize };
    ui::PedalKnob distnKnob { "DISTORTION", BinaryData::knob_large_strip_png, BinaryData::knob_large_strip_pngSize };
    ui::PedalKnob outKnob { "OUTPUT", BinaryData::knob_small_strip_png, BinaryData::knob_small_strip_pngSize };
    juce::ToggleButton darkToggle { "Dark" };
    juce::ToggleButton gateToggle { "Gate Post" };
    ui::ClipLed clipLed;
    ui::PressureSendPad pressurePad;
    ui::TransparentHitButton advancedButton { "Advanced" };
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
    juce::TooltipWindow tooltipWindow { this, 650 };

    void loadPresetFromDisk();
    void savePresetToDisk();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};

} // namespace sendbloom
