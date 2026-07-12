#pragma once

#include "PedalKnob.h"
#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace sendbloom::ui
{

class AdvancedDrawer : public juce::Component
{
public:
    AdvancedDrawer (juce::AudioProcessorValueTreeState& apvts,
                    const juce::String& thresholdId,
                    const juce::String& sendFeelId,
                    const juce::String& authenticColorId,
                    const juce::String& extendedStereoId,
                    const juce::String& sendConnectedId);

    void setExpanded (bool shouldExpand);
    bool isExpanded() const noexcept { return expanded; }

    int getPreferredHeight() const noexcept { return expanded ? 204 : 0; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    bool expanded { false };
    PedalKnob gateSensKnob { "Gate Sens", BinaryData::knob_distortion_png,
                             static_cast<size_t> (BinaryData::knob_distortion_pngSize) };
    juce::ComboBox sendFeelBox;
    juce::Label sendFeelLabel;
    juce::ToggleButton pressureModeToggle { "PRESSURE MODE" };
    juce::ToggleButton colorToggle { "32k Color" };
    juce::ToggleButton extendedStereoToggle { "Extended Stereo" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateSensAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sendFeelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pressureModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> extendedStereoAttachment;
};

} // namespace sendbloom::ui
