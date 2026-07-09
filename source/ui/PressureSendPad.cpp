#include "PressureSendPad.h"

namespace sendbloom::ui
{

PressureSendPad::PressureSendPad (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& connectedParamId,
                                  const juce::String& amountParamId)
{
    connectedParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (connectedParamId));
    amountParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (amountParamId));
    setOpaque (false);
}

void PressureSendPad::paint (juce::Graphics&)
{
    // Faceplate art + editor overlays draw the footswitch; this component is the hit target.
}

void PressureSendPad::mouseDown (const juce::MouseEvent& e)
{
    stopBloomFade();
    isPressed = true;
    touchPoint = e.position;
    setConnected (true);
    setAmountFromY (e.position.y);
    if (auto* parent = getParentComponent())
        parent->repaint();
}

void PressureSendPad::mouseDrag (const juce::MouseEvent& e)
{
    touchPoint = e.position;
    setAmountFromY (e.position.y);
    if (auto* parent = getParentComponent())
        parent->repaint();
}

void PressureSendPad::mouseUp (const juce::MouseEvent&)
{
    isPressed = false;
    setConnected (false);
    startBloomFade();
    if (auto* parent = getParentComponent())
        parent->repaint();
}

void PressureSendPad::timerCallback()
{
    const auto elapsed = juce::Time::getMillisecondCounterHiRes() - fadeStartTimeMs;
    const auto progress = juce::jlimit (0.0, 1.0, elapsed / static_cast<double> (kBloomFadeMs));
    displayAmount = fadeStartAmount * static_cast<float> (1.0 - progress);

    if (auto* parent = getParentComponent())
        parent->repaint();

    if (progress >= 1.0)
        stopBloomFade();
}

void PressureSendPad::setConnected (bool connected)
{
    if (connectedParam != nullptr)
        connectedParam->setValueNotifyingHost (connected ? 1.0f : 0.0f);
}

void PressureSendPad::setAmountFromY (float y)
{
    if (amountParam == nullptr)
        return;

    const auto bounds = getLocalBounds().toFloat().reduced (4.0f);
    const auto norm = juce::jlimit (0.0f, 1.0f, 1.0f - (y - bounds.getY()) / bounds.getHeight());
    amountParam->setValueNotifyingHost (norm);
    displayAmount = norm;
}

void PressureSendPad::startBloomFade()
{
    if (displayAmount <= 0.001f)
    {
        displayAmount = 0.0f;
        return;
    }

    fadeStartAmount = displayAmount;
    fadeStartTimeMs = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void PressureSendPad::stopBloomFade()
{
    stopTimer();
    displayAmount = 0.0f;
}

} // namespace sendbloom::ui
