#include "PressureSendPad.h"
#include "PedalFaceplatePaint.h"

namespace sendbloom::ui
{

namespace
{
// Smoothstep. Takes the linear ramp off both ends so the cap settles rather
// than arriving at full speed.
float ease (float t) noexcept
{
    return t * t * (3.0f - 2.0f * t);
}
} // namespace

PressureSendPad::PressureSendPad (juce::AudioProcessorValueTreeState& apvts,
                                  const juce::String& connectedParamId,
                                  const juce::String& amountParamId)
{
    connectedParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (connectedParamId));
    amountParam = dynamic_cast<juce::RangedAudioParameter*> (apvts.getParameter (amountParamId));
    setOpaque (false);
    setWantsKeyboardFocus (true);
    setName ("Pressure send");
    setTitle ("Pressure send");
    setDescription ("Momentary pressure control for the reverb send amount");
    setHelpText ("Press Space or Return to engage or release. Use the arrow keys to adjust pressure.");
}

void PressureSendPad::paint (juce::Graphics&)
{
    // Faceplate art + editor overlays draw the footswitch; this component is the hit target.
}

void PressureSendPad::mouseDown (const juce::MouseEvent& e)
{
    stopBloomFade();
    pressed = true;
    touchPoint = e.position;
    setConnected (true);
    beginAmountGesture();
    setAmountFromY (e.position.y);
    startTimerHz (60); // drive the cap down
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
    releasePressure();
}

bool PressureSendPad::keyPressed (const juce::KeyPress& key)
{
    if (key == juce::KeyPress::spaceKey || key == juce::KeyPress::returnKey)
    {
        toggleKeyboardPressure();
        return true;
    }

    if (key == juce::KeyPress::upKey || key == juce::KeyPress::rightKey)
    {
        const auto current = amountParam != nullptr ? amountParam->getValue() : 0.0f;
        engageFromKeyboard (current + 0.05f);
        return true;
    }

    if (key == juce::KeyPress::downKey || key == juce::KeyPress::leftKey)
    {
        const auto current = amountParam != nullptr ? amountParam->getValue() : 0.0f;
        const auto next = juce::jmax (0.0f, current - 0.05f);
        if (next <= 0.0f)
            releasePressure();
        else
            engageFromKeyboard (next);
        return true;
    }

    if (key == juce::KeyPress::homeKey)
    {
        releasePressure();
        return true;
    }

    if (key == juce::KeyPress::endKey)
    {
        engageFromKeyboard (1.0f);
        return true;
    }

    return false;
}

void PressureSendPad::focusLost (juce::Component::FocusChangeType)
{
    if (pressed)
        releasePressure();
}

class PressureSendPad::AccessibilityValue final
    : public juce::AccessibilityRangedNumericValueInterface
{
public:
    explicit AccessibilityValue (PressureSendPad& ownerToUse) : owner (ownerToUse) {}

    bool isReadOnly() const override { return false; }

    double getCurrentValue() const override
    {
        return owner.amountParam != nullptr
                 ? static_cast<double> (owner.amountParam->getValue()) * 100.0
                 : 0.0;
    }

    void setValue (double newValue) override
    {
        const auto amount = juce::jlimit (0.0f, 1.0f, static_cast<float> (newValue / 100.0));
        if (amount <= 0.0f)
            owner.releasePressure();
        else
            owner.engageFromKeyboard (amount);
    }

    AccessibleValueRange getRange() const override
    {
        return { { 0.0, 100.0 }, 5.0 };
    }

private:
    PressureSendPad& owner;
};

std::unique_ptr<juce::AccessibilityHandler> PressureSendPad::createAccessibilityHandler()
{
    juce::AccessibilityActions actions;
    actions.addAction (juce::AccessibilityActionType::press,
                       [this] { toggleKeyboardPressure(); });

    return std::make_unique<juce::AccessibilityHandler> (
        *this,
        juce::AccessibilityRole::slider,
        std::move (actions),
        juce::AccessibilityHandler::Interfaces { std::make_unique<AccessibilityValue> (*this) });
}

void PressureSendPad::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();

    if (fadeActive)
    {
        const auto progress = juce::jlimit (0.0, 1.0, (now - fadeStartTimeMs)
                                                          / static_cast<double> (kBloomFadeMs));
        displayAmount = fadeStartAmount * static_cast<float> (1.0 - progress);

        if (progress >= 1.0)
        {
            fadeActive = false;
            displayAmount = 0.0f;
        }
    }

    // Re-target every frame: the switch should stay down for as long as the
    // overlay predicate says the send is live, which outlasts the mouse by the
    // length of the bloom fade. Only once that clears does the cap spring back.
    retargetTravel();
    const auto travelling = advanceTravel (now);

    if (auto* parent = getParentComponent())
        parent->repaint();

    if (! fadeActive && ! travelling)
        stopTimer();
}

void PressureSendPad::retargetTravel()
{
    const auto amountNorm = amountParam != nullptr ? amountParam->getValue() : 0.0f;
    const auto target = shouldDrawFootswitchPressedOverlay (pressed, displayAmount, amountNorm)
                          ? 1.0f
                          : 0.0f;

    if (target == travelTarget)
        return;

    travelFrom = pressTravel;
    travelTarget = target;
    travelStartMs = juce::Time::getMillisecondCounterHiRes();
    travelDurationMs = target > pressTravel ? kPressDownMs : kPressUpMs;
}

/** Advances the eased travel; returns true while there is still travel to do. */
bool PressureSendPad::advanceTravel (double nowMs)
{
    if (travelDurationMs <= 0)
    {
        pressTravel = travelTarget;
        return false;
    }

    const auto progress = juce::jlimit (0.0, 1.0, (nowMs - travelStartMs)
                                                      / static_cast<double> (travelDurationMs));
    pressTravel = travelFrom + (travelTarget - travelFrom) * ease (static_cast<float> (progress));
    return progress < 1.0;
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
    setAmountNormalized (norm);
}

void PressureSendPad::setAmountNormalized (float amount)
{
    if (amountParam == nullptr)
        return;

    const auto norm = juce::jlimit (0.0f, 1.0f, amount);
    amountParam->setValueNotifyingHost (norm);
    displayAmount = norm;
    notifyAmountChanged();
}

void PressureSendPad::engageFromKeyboard (float amount)
{
    stopBloomFade();
    pressed = true;
    setConnected (true);
    beginAmountGesture();
    setAmountNormalized (juce::jmax (0.05f, amount));
    endAmountGesture();
    startTimerHz (60);

    if (auto* parent = getParentComponent())
        parent->repaint();
}

void PressureSendPad::releasePressure()
{
    // SEND-05 / ADR-V1-02: release zeros amount and leaves send_connected true.
    pressed = false;

    if (amountParam != nullptr)
    {
        if (! amountGestureActive)
            beginAmountGesture();

        amountParam->setValueNotifyingHost (0.0f);
        endAmountGesture();
        notifyAmountChanged();
    }

    // Bloom fade is display-only (not DSP release); start from last visual amount.
    startBloomFade();
    if (auto* parent = getParentComponent())
        parent->repaint();
}

void PressureSendPad::toggleKeyboardPressure()
{
    if (pressed)
        releasePressure();
    else
        engageFromKeyboard (1.0f);
}

void PressureSendPad::notifyAmountChanged()
{
    if (auto* handler = getAccessibilityHandler())
        handler->notifyAccessibilityEvent (juce::AccessibilityEvent::valueChanged);
}

void PressureSendPad::beginAmountGesture()
{
    if (amountParam == nullptr || amountGestureActive)
        return;

    amountParam->beginChangeGesture();
    amountGestureActive = true;
}

void PressureSendPad::endAmountGesture()
{
    if (amountParam == nullptr || ! amountGestureActive)
        return;

    amountParam->endChangeGesture();
    amountGestureActive = false;
}

void PressureSendPad::startBloomFade()
{
    if (displayAmount <= 0.001f)
    {
        displayAmount = 0.0f;
        fadeActive = false;
    }
    else
    {
        fadeStartAmount = displayAmount;
        fadeStartTimeMs = juce::Time::getMillisecondCounterHiRes();
        fadeActive = true;
    }

    // Run regardless of the fade: the cap still has to travel back up.
    startTimerHz (60);
}

void PressureSendPad::stopBloomFade()
{
    fadeActive = false;
    displayAmount = 0.0f;
}

} // namespace sendbloom::ui
