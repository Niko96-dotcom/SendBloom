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
                    const juce::String& dirtOsId);

    void setExpanded (bool shouldExpand);
    bool isExpanded() const noexcept { return expanded; }

    int getPreferredHeight() const noexcept { return expanded ? 160 : 0; }

    void resized() override;

private:
    bool expanded { false };
    PedalKnob gateSensKnob { "Gate Sens" };
    juce::ComboBox sendFeelBox;
    juce::Label sendFeelLabel;
    juce::ToggleButton colorToggle { "32k Color" };
    juce::ToggleButton extendedStereoToggle { "Extended Stereo" };
    juce::ToggleButton dirtOsToggle { "Dirt OS" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateSensAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sendFeelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> colorAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> extendedStereoAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> dirtOsAttachment;
};

} // namespace sendbloom::ui
