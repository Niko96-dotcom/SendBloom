#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace sendbloom::ui
{

class PressureSendPad : public juce::Component
{
public:
    PressureSendPad (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& connectedParamId,
                     const juce::String& amountParamId);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

private:
    void setConnected (bool connected);
    void setAmountFromY (float y);
    float currentAmount() const;

    juce::RangedAudioParameter* connectedParam { nullptr };
    juce::RangedAudioParameter* amountParam { nullptr };
    juce::Point<float> touchPoint;
    bool isPressed { false };
    float displayAmount { 0.0f };
};

} // namespace sendbloom::ui
