#pragma once

#include "ParameterCurves.h"
#include <algorithm>
#include <cmath>

namespace sendbloom
{

/** Owns connected/rest send-gain truth: max(host, midi) → asymmetric smooth → Firm/Soft curve.
 *  Disconnected returns unity (always-on wet). Phase 20: callers leave MIDI target at 0.
 */
class PressureController
{
public:
    void prepare (double sampleRate) noexcept
    {
        sampleRate_ = sampleRate > 0.0 ? sampleRate : 48000.0;
        updateCoeffs();
        reset();
    }

    void reset() noexcept
    {
        smoothedPressure_ = 0.0f;
    }

    /** Snap smoother to the current host/midi max (prepare / program load — no attack ramp). */
    void snapToTarget() noexcept
    {
        smoothedPressure_ = juce::jlimit (0.0f, 1.0f, std::max (hostTarget_, midiTarget_));
    }

    void setConnected (bool connected) noexcept
    {
        connected_ = connected;
    }

    void setHostPressureTarget (float normalized) noexcept
    {
        hostTarget_ = juce::jlimit (0.0f, 1.0f, normalized);
    }

    void setMidiPressureTarget (float normalized) noexcept
    {
        midiTarget_ = juce::jlimit (0.0f, 1.0f, normalized);
    }

    void setFirmFeel (bool firm) noexcept
    {
        firmFeel_ = firm;
    }

    float processSample() noexcept
    {
        if (! connected_)
            return 1.0f;

        const auto rawTarget = std::max (hostTarget_, midiTarget_);
        const auto coeff = rawTarget > smoothedPressure_ ? attackCoeff_ : releaseCoeff_;
        smoothedPressure_ += coeff * (rawTarget - smoothedPressure_);

        return ParameterCurves::sendGain (smoothedPressure_, firmFeel_);
    }

private:
    void updateCoeffs() noexcept
    {
        // One-pole: y += (1 - e^{-1/(τ·fs)}) · (x - y); τ = time to ~63% of step.
        constexpr auto kAttackSeconds = 0.003;
        constexpr auto kReleaseSeconds = 0.025;
        attackCoeff_ = static_cast<float> (1.0 - std::exp (-1.0 / (kAttackSeconds * sampleRate_)));
        releaseCoeff_ = static_cast<float> (1.0 - std::exp (-1.0 / (kReleaseSeconds * sampleRate_)));
    }

    double sampleRate_ { 48000.0 };
    float attackCoeff_ { 0.0f };
    float releaseCoeff_ { 0.0f };
    float hostTarget_ { 0.0f };
    float midiTarget_ { 0.0f };
    float smoothedPressure_ { 0.0f };
    bool connected_ { false };
    bool firmFeel_ { true };
};

} // namespace sendbloom
