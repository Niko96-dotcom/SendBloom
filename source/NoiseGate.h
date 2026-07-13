#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <algorithm>
#include <cmath>

namespace sendbloom
{

enum class GateProfile
{
    PreSoft,
    PostHard
};

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

    void prepare (double sampleRate, GateProfile profile) noexcept
    {
        sampleRate_ = sampleRate;
        gain = 1.0f;
        isOpen = true;
        holdCounter = 0;
        configureProfile (profile);
    }

    // Re-point the gate at a different profile WITHOUT resetting the running
    // open/close/hold/gain state. This is how a single shared gate "moves"
    // between the pre- and post-effect positions like the hardware's one physical
    // circuit, instead of two independent gates that surface stale state when the
    // Gate switch is toggled.
    void setProfile (GateProfile profile) noexcept
    {
        if (profile == profile_)
            return;

        configureProfile (profile);
    }

    float process (float inputEnvelope, float thresholdDb) noexcept
    {
        const auto openThresh = juce::Decibels::decibelsToGain (thresholdDb);
        return processLinear (inputEnvelope, openThresh);
    }

    float processLinear (float inputEnvelope, float openThreshold) noexcept
    {
        constexpr auto kCloseThresholdRatio = 0.7943282347f; // -2 dB
        const auto openThresh = juce::jmax (0.0f, openThreshold);
        const auto closeThresh = openThresh * kCloseThresholdRatio;

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

        if (postHardClose)
        {
            if (isOpen)
                // Linear ~0.2 ms attack: preserves the violent front edge of the
                // wet burst instead of a one-pole that rounds the transient away.
                gain = std::min (1.0f, gain + openRampStep);
            else
                // Deterministic linear ~0.75 ms chop to zero.
                gain = std::max (0.0f, gain - closeRampStep);

            return gain;
        }

        const auto target = isOpen ? 1.0f : floorGain;
        const auto coeff = target > gain ? attackCoeff : releaseCoeff;
        gain = coeff * gain + (1.0f - coeff) * target;
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
    void configureProfile (GateProfile profile) noexcept
    {
        profile_ = profile;

        switch (profile)
        {
            case GateProfile::PreSoft:
                releaseMs = 150.0f;
                attackMs = 2.0f;
                floorGain = juce::Decibels::decibelsToGain (-80.0f);
                postHardClose = false;
                break;

            case GateProfile::PostHard:
                // ADR-V1-11: deterministic hard chop; fast linear edges.
                releaseMs = 0.75f; // close ramp length
                attackMs = 0.2f;   // open ramp length (linear, not one-pole)
                floorGain = 0.0f;
                postHardClose = true;
                break;
        }

        releaseCoeff = coeffForMs (releaseMs, sampleRate_);
        attackCoeff = coeffForMs (attackMs, sampleRate_);
        openRampStep = rampStepForMs (attackMs, sampleRate_);
        closeRampStep = rampStepForMs (releaseMs, sampleRate_);
        holdSamples = static_cast<int> (std::max (1.0, kHoldMs * 0.001 * sampleRate_));
    }

    static float coeffForMs (float ms, double sampleRate) noexcept
    {
        return std::exp (-1.0f / (ms * 0.001f * static_cast<float> (sampleRate)));
    }

    static float rampStepForMs (float ms, double sampleRate) noexcept
    {
        const auto samples = std::max (1.0, ms * 0.001 * sampleRate);
        return static_cast<float> (1.0 / samples);
    }

    float gain { 1.0f };
    float floorGain { 0.0f };
    float attackCoeff { 0.0f };
    float releaseCoeff { 0.0f };
    float attackMs { 2.0f };
    float releaseMs { 150.0f };
    float openRampStep { 0.0f };
    float closeRampStep { 0.0f };
    int holdSamples { 0 };
    int holdCounter { 0 };
    double sampleRate_ { 48000.0 };
    bool isOpen { true };
    bool postHardClose { false };
    GateProfile profile_ { GateProfile::PreSoft };
};

} // namespace sendbloom
