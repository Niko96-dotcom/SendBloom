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
                    const juce::String& extendedStereoId,
                    const juce::String& sendConnectedId);

    ~AdvancedDrawer() override;
    void setExpanded (bool shouldExpand);
    bool isExpanded() const noexcept { return expanded; }

    int getPreferredHeight() const noexcept { return expanded ? 150 : 0; }

    void paint (juce::Graphics& g) override;
    void resized() override;

private:
    class ServiceLookAndFeel final : public juce::LookAndFeel_V4
    {
        void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool hover, bool down) override
        {
            const auto height = juce::jmin(34.0f, float(button.getHeight()) - 2.0f);
            const auto width = height * (58.0f / 34.0f);
            {
                juce::Graphics::ScopedSaveState state(g);
                g.reduceClipRegion(juce::Rectangle<float>(0,0,width,float(button.getHeight())).getSmallestIntegerContainer());
                button.getParentComponent()->getLookAndFeel().drawToggleButton(g,button,hover,down);
            }
            auto text = button.getLocalBounds().toFloat().withTrimmedLeft(width + 5.0f);
            g.setColour(niko::clear::palette::ink);
            g.setFont(niko::clear::sans(12.0f,true));
            g.drawFittedText(button.getButtonText().toUpperCase(),text.withHeight(17.0f).toNearestInt(),juce::Justification::centredLeft,1);
            g.setColour(niko::clear::palette::muted);
            g.setFont(niko::clear::sans(12.0f));
            g.drawText(button.getToggleState() ? "ON" : "OFF",text.withTrimmedTop(17.0f),juce::Justification::centredLeft);
        }
    } serviceLookAndFeel;
    bool expanded { false };
    PedalKnob gateSensKnob { "GATE TRIM", BinaryData::knob_small_strip_png,
                             static_cast<size_t> (BinaryData::knob_small_strip_pngSize) };
    juce::ComboBox sendFeelBox;
    juce::Label sendFeelLabel;
    juce::ToggleButton pressureModeToggle { "PRESSURE MODE" };
    juce::ToggleButton extendedStereoToggle { "Extended Stereo" };

    std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> gateSensAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> sendFeelAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> pressureModeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> extendedStereoAttachment;
};

} // namespace sendbloom::ui
