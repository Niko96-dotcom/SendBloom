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

    /** How far the switch has travelled: 0 raised, 1 fully stomped, eased so the
        cap moves instead of teleporting between the two states.

        Returns -1 when no animation is in flight, meaning "nothing to say — work
        it out yourself". Travel is only tracked while the timer runs, so a send
        engaged by automation or a preset (no mouse, no timer) would otherwise
        read as raised. Settled travel always agrees with the overlay predicate,
        so handing the decision back costs nothing. */
    float getPressTravel() const noexcept { return isTimerRunning() ? pressTravel : -1.0f; }

private:
    void timerCallback() override;
    void setConnected (bool connected);
    void setAmountFromY (float y);
    void beginAmountGesture();
    void endAmountGesture();
    void startBloomFade();
    void stopBloomFade();
    void retargetTravel();
    bool advanceTravel (double nowMs);

    static constexpr int kBloomFadeMs = 200;
    // Down fast, back up slower: a stomp is a hard stop, the release is a spring.
    static constexpr int kPressDownMs = 60;
    static constexpr int kPressUpMs = 140;

    juce::RangedAudioParameter* connectedParam { nullptr };
    juce::RangedAudioParameter* amountParam { nullptr };
    juce::Point<float> touchPoint;
    bool pressed { false };
    bool amountGestureActive { false };
    float displayAmount { 0.0f };
    float fadeStartAmount { 0.0f };
    double fadeStartTimeMs { 0.0 };
    bool fadeActive { false };
    float pressTravel { 0.0f };
    float travelFrom { 0.0f };
    float travelTarget { 0.0f };
    double travelStartMs { 0.0 };
    int travelDurationMs { kPressDownMs };
};

} // namespace sendbloom::ui
