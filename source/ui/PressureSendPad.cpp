#include "PressureSendPad.h"
#include "SendBloomLookAndFeel.h"

namespace sendbloom::ui
{

PressureSendPad::PressureSendPad (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& connectedParamId,
                                  const juce::String& amountParamId)
{
    connectedParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (connectedParamId));
    amountParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (amountParamId));
}

void PressureSendPad::paint (juce::Graphics& g)
{
    const auto bounds = getLocalBounds().toFloat().reduced (4.0f);

    g.setColour (juce::Colour (0xff3D3D42));
    g.fillRoundedRectangle (bounds, 8.0f);
    g.setColour (juce::Colour (0xff4A4A50));
    g.drawRoundedRectangle (bounds, 8.0f, 1.5f);

    if (displayAmount > 0.001f || isPressed)
    {
        const auto centre = isPressed ? touchPoint : bounds.getCentre();
        const auto radius = bounds.getWidth() * 0.5f * displayAmount;

        juce::ColourGradient bloom (juce::Colour (0xffE8A838).withAlpha (0.55f * displayAmount),
                                    centre.x, centre.y,
                                    juce::Colours::transparentBlack,
                                    centre.x, centre.y + radius,
                                    true);
        g.setGradientFill (bloom);
        g.fillEllipse (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f);
    }

    g.setColour (juce::Colour (0xffE8E6E3).withAlpha (0.7f));
    g.setFont (juce::FontOptions (10.0f));
    g.drawText ("PRESS", bounds, juce::Justification::centred);
}

void PressureSendPad::mouseDown (const juce::MouseEvent& e)
{
    stopBloomFade();
    isPressed = true;
    touchPoint = e.position;
    setConnected (true);
    setAmountFromY (e.position.y);
    repaint();
}

void PressureSendPad::mouseDrag (const juce::MouseEvent& e)
{
    touchPoint = e.position;
    setAmountFromY (e.position.y);
    repaint();
}

void PressureSendPad::mouseUp (const juce::MouseEvent&)
{
    isPressed = false;
    setConnected (false);
    startBloomFade();
}

void PressureSendPad::timerCallback()
{
    const auto elapsed = juce::Time::getMillisecondCounterHiRes() - fadeStartTimeMs;
    const auto progress = juce::jlimit (0.0, 1.0, elapsed / static_cast<double> (kBloomFadeMs));
    displayAmount = fadeStartAmount * static_cast<float> (1.0 - progress);
    repaint();

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
        repaint();
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

float PressureSendPad::currentAmount() const
{
    if (amountParam == nullptr)
        return 0.0f;

    return amountParam->getValue();
}

} // namespace sendbloom::ui
