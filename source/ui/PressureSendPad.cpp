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
    auto bounds = getLocalBounds().toFloat();
    const auto top = bounds.removeFromTop (96.0f);
    const auto switchCentre = top.getCentre().translated (0.0f, isPressed ? 5.0f : 0.0f);
    const auto amountGlow = juce::jmax (displayAmount, isPressed ? 0.78f : 0.0f);

    juce::ColourGradient base (juce::Colour (0xff272727), top.getX(), top.getY(),
                               juce::Colour (0xff070707), top.getRight(), top.getBottom(), false);
    g.setGradientFill (base);
    g.fillRoundedRectangle (top.reduced (4.0f), 6.0f);

    if (amountGlow > 0.001f)
    {
        juce::ColourGradient glow (juce::Colour (0xff8df24f).withAlpha (0.42f * amountGlow),
                                   switchCentre.x + 58.0f, switchCentre.y - 22.0f,
                                   juce::Colours::transparentBlack,
                                   switchCentre.x + 58.0f, switchCentre.y + 28.0f,
                                   true);
        g.setGradientFill (glow);
        g.fillEllipse (switchCentre.x + 38.0f, switchCentre.y - 42.0f, 42.0f, 42.0f);
    }

    const auto led = juce::Rectangle<float> (switchCentre.x + 52.0f, switchCentre.y - 28.0f, 18.0f, 18.0f);
    g.setColour (juce::Colour (0xff27301f));
    g.fillEllipse (led.expanded (4.0f));
    juce::ColourGradient ledFill (isPressed ? juce::Colour (0xffb7ff67) : juce::Colour (0xff83df4a),
                                  led.getX(), led.getY(),
                                  isPressed ? juce::Colour (0xff3eb125) : juce::Colour (0xff376d2c),
                                  led.getRight(), led.getBottom(), false);
    g.setGradientFill (ledFill);
    g.fillEllipse (led);
    g.setColour (juce::Colours::white.withAlpha (0.36f));
    g.fillEllipse (led.reduced (5.0f).translated (-2.0f, -2.0f));

    const auto ringOuter = juce::Rectangle<float> (switchCentre.x - 46.0f, switchCentre.y - 41.0f, 92.0f, 92.0f);
    juce::ColourGradient shadow (juce::Colours::black.withAlpha (0.55f), ringOuter.getX(), ringOuter.getBottom(),
                                 juce::Colours::transparentBlack, ringOuter.getX(), ringOuter.getY(), false);
    g.setGradientFill (shadow);
    g.fillEllipse (ringOuter.translated (0.0f, 8.0f));

    for (int i = 0; i < 4; ++i)
    {
        const auto ring = ringOuter.reduced (static_cast<float> (i * 7));
        juce::ColourGradient ringFill (juce::Colour (0xfff0f0ed).darker (0.08f * static_cast<float> (i)),
                                       ring.getX(), ring.getY(),
                                       juce::Colour (0xff777777).darker (0.08f * static_cast<float> (i)),
                                       ring.getRight(), ring.getBottom(), false);
        g.setGradientFill (ringFill);
        g.fillEllipse (ring);
        g.setColour (juce::Colour (0xff2b2b2b));
        g.drawEllipse (ring, 1.4f);
    }

    const auto cap = ringOuter.reduced (28.0f).translated (0.0f, isPressed ? 4.0f : 0.0f);
    juce::ColourGradient capFill (isPressed ? juce::Colour (0xffbdbdb9) : juce::Colour (0xfff3f3f0),
                                  cap.getX(), cap.getY(),
                                  juce::Colour (0xff919191), cap.getRight(), cap.getBottom(), false);
    g.setGradientFill (capFill);
    g.fillEllipse (cap);
    g.setColour (juce::Colour (0xff4c4c4c));
    g.drawEllipse (cap, 2.0f);
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
    repaint();
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

} // namespace sendbloom::ui
