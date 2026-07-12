#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>

namespace sendbloom::ui
{

class PressureSendPad : public juce::Component, private juce::Timer
{
public:
    PressureSendPad (juce::AudioProcessorValueTreeState& apvts,
                     const juce::String& connectedParamId,
                     const juce::String& amountParamId);

    void paint (juce::Graphics& g) override;
    void mouseDown (const juce::MouseEvent& e) override;
    void mouseDrag (const juce::MouseEvent& e) override;
    void mouseUp (const juce::MouseEvent& e) override;

    bool isPressed() const noexcept { return pressed; }
    float getDisplayAmount() const noexcept { return displayAmount; }

private:
    void timerCallback() override;
    void setConnected (bool connected);
    void setAmountFromY (float y);
    void beginAmountGesture();
    void endAmountGesture();
    void startBloomFade();
    void stopBloomFade();

    static constexpr int kBloomFadeMs = 200;

    juce::RangedAudioParameter* connectedParam { nullptr };
    juce::RangedAudioParameter* amountParam { nullptr };
    juce::Point<float> touchPoint;
    bool pressed { false };
    bool amountGestureActive { false };
    float displayAmount { 0.0f };
    float fadeStartAmount { 0.0f };
    double fadeStartTimeMs { 0.0 };
};

} // namespace sendbloom::ui
