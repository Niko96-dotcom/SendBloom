#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

namespace sendbloom
{

/** One gate circuit, one envelope.

    The reference hardware does not swap in a gentler gate for the pre
    position: the manufacturer describes a gate that is always in circuit, has
    a trigger threshold set low enough to go unnoticed ahead of the effect, and
    a closing speed that is not adjustable. The Gate switch *moves* that one
    circuit from ahead of the reverb/distortion to behind it. Placement
    therefore lives in GatedBloomChain, and this class has no per-position
    profile.

    ADR-V1-11c removed the former PreSoft profile (2 ms one-pole open, 150 ms
    one-pole release to a -80 dB floor). Measured, it bought nothing: with the
    gate ahead of the tank the tail after a mute was identical to the fast
    profile within 0.07 dB at every point out to 1.9 s, because once the gate
    stops feeding the tank the tank's own decay dominates. What it cost was the
    job the manual actually gives the gate — taming hum into the reverb and
    distortion stages. The slow profile was only 5 dB down 100 ms into a gap and
    took 705 ms to reach -40 dB, leaving roughly 30 dB more hum in the tank
    across the first half second of every gap, where it then reverberates for
    the length of the tail. Its 2 ms one-pole open also rounded the front edge
    off every note feeding the bloom — the same defect ADR-V1-11a removed from
    the post position.
*/
class NoiseGate
{
public:
    // Gap between the open and close thresholds. Tightened from the original 3 dB
    // so playing right around the boundary can sputter — the "well-tuned" on/off
    // character owners describe on the reference hardware — without full chatter.
    static constexpr float kHysteresisDb = 2.0f;

    // Hold time: once the key drops below the close threshold the gate stays open
    // for this long before closing. Bridges the sub-threshold window between the
    // peaks of a low note (and the micro-gaps between pick attacks) so a sustained
    // riff doesn't stutter with the fast detector release, while a real mute still
    // resolves quickly. 5 ms clears the ~2.7 ms below-threshold window of an
    // unclipped low E (82 Hz) with margin; measured stutter-free down to INPT 0.35.
    static constexpr double kHoldMs = 5.0;

    // ADR-V1-11 / 11a: deterministic linear edges. The opening ramp is linear
    // rather than a one-pole so it preserves the violent front edge of the wet
    // burst; the closing ramp is a deterministic chop to exact zero that is fast
    // enough to read as an edit but long enough not to be a one-sample click.
    static constexpr float kOpenRampMs = 0.2f;
    static constexpr float kCloseRampMs = 0.75f;

    void prepare (double sampleRate) noexcept
    {
        sampleRate_ = sampleRate;
        configure();
        reset();
    }

    float process (float inputEnvelope, float thresholdDb) noexcept
    {
        return processLinear (inputEnvelope, juce::Decibels::decibelsToGain (thresholdDb));
    }

    float processLinear (float inputEnvelope, float openThreshold) noexcept
    {
        const auto openThresh = juce::jmax (0.0f, openThreshold);
        const auto closeThresh = openThresh * closeThresholdRatio;

        // Trigger + hold state machine. Opening is instant; closing waits out the
        // hold so brief dropouts don't retrigger, but a genuine mute closes.
        if (inputEnvelope > openThresh)
            isOpen = true;

        if (inputEnvelope >= closeThresh)
            holdCounter = holdSamples;   // signal present: keep the hold armed
        else if (holdCounter > 0)
            --holdCounter;               // dropped out: ride the hold window
        else
            isOpen = false;              // hold expired: close

        gain = isOpen ? std::min (1.0f, gain + openRampStep)
                      : std::max (0.0f, gain - closeRampStep);

        return gain;
    }

    bool getIsOpen() const noexcept { return isOpen; }
    float getGain() const noexcept { return gain; }

    void reset() noexcept
    {
        gain = 1.0f;
        isOpen = true;
        holdCounter = 0;
    }

private:
    void configure() noexcept
    {
        openRampStep = rampStepForMs (kOpenRampMs, sampleRate_);
        closeRampStep = rampStepForMs (kCloseRampMs, sampleRate_);
        holdSamples = static_cast<int> (std::max (1.0, kHoldMs * 0.001 * sampleRate_));
        closeThresholdRatio = juce::Decibels::decibelsToGain (-kHysteresisDb);
    }

    static float rampStepForMs (float ms, double sampleRate) noexcept
    {
        const auto samples = std::max (1.0, ms * 0.001 * sampleRate);
        return static_cast<float> (1.0 / samples);
    }

    float gain { 1.0f };
    float openRampStep { 0.0f };
    float closeRampStep { 0.0f };
    float closeThresholdRatio { 1.0f };
    int holdSamples { 0 };
    int holdCounter { 0 };
    double sampleRate_ { 48000.0 };
    bool isOpen { true };
};

} // namespace sendbloom
