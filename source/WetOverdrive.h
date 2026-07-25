#pragma once

#include <cmath>

namespace sendbloom
{

enum class OverdriveCurve
{
    Legacy = 0,
    TamedTanh = 1,
    DiodeSoftClip = 2,
    CubicSoftClip = 3,
    BarrReciprocal = 4
};

struct WetOverdrive
{
    // Spin Semiconductor's design notes name the nonlinearity to use for
    // guitar distortion on this hardware class, and why:
    //
    //   "If X<1 then Y=X / If X>1 then Y=2 - 1/X ... The sound however,
    //    especially for a guitar and keyboard instruments, is very nice, and
    //    aliasing is largely avoided. This concept delivers nice distortion,
    //    that is, lower level signals are clean, and only 'break up' on
    //    transients and emphasized instrumental notes."
    //
    // That last property is the audible difference from the tanh curve this
    // replaces. tanh bends everywhere: its gain fell continuously from 1.95x on
    // near-silent input to 0.92x at full scale, so it squashed the whole reverb
    // tail uniformly. The reciprocal curve is exactly linear below threshold and
    // only rounds above it, so quiet trails stay clean and only the blooms bite
    // — and being a 1/x form it generates mostly low-order harmonics, which is
    // why Spin recommends it over curves that alias badly at 32 kHz.
    static constexpr OverdriveCurve kActiveCurve = OverdriveCurve::BarrReciprocal;

    static constexpr float kDriveBarr = 3.0f;
    // Mild positive-side asymmetry for even-harmonic warmth; the bare function
    // is symmetric, real pedal clipping stages are not.
    static constexpr float kAsymPosBarr = 1.10f;
    // Flat trim. Bracketed from both sides: the dirty branch has to read as
    // *added* rather than as a level drop (GatedBloomChainTest), while the wet
    // tail must not swell when DISTN opens (WetOverdriveDiagnosticsTest's 1.15
    // ceiling, milestone spec 13.7) — otherwise DISTN doubles as a volume knob.
    // 1.20 puts the 220 Hz tail ratio at ~1.06, inside that window.
    //
    // Being flat, it leaves the part that matters alone: below threshold the
    // curve is still exactly straight, so quiet trails get level but no
    // harmonics. The old curve had no straight region at all.
    static constexpr float kMakeupBarr = 1.20f;

    // Candidate A — tamed asymmetric tanh with level-dependent drive
    static constexpr float kDriveA = 2.05f;
    static constexpr float kDriveQuietA = 1.10f;
    static constexpr float kAsymPosA = 1.08f;
    static constexpr float kMakeupA = 0.92f;
    static constexpr float kMakeupQuietA = 0.72f;
    static constexpr float kQuietReferenceA = 0.10f;

    // Legacy reference (pre-voicing)
    static constexpr float kDriveLegacy = 3.0f;
    static constexpr float kAsymPosLegacy = 1.12f;

    // Candidate B — asymmetric diode-style soft clip
    static constexpr float kPreGainB = 2.1f;
    static constexpr float kPosThresholdB = 0.52f;
    static constexpr float kNegThresholdB = 0.62f;
    static constexpr float kKneeB = 0.10f;
    static constexpr float kMakeupB = 0.68f;

    // Candidate C — bounded cubic soft clip
    static constexpr float kPreGainC = 1.75f;
    static constexpr float kMakeupC = 0.72f;

    // Tone shaping (dirty branch only)
    static constexpr float kPreClipLpHz = 6500.0f;
    static constexpr float kPreClipHpHz = 100.0f;
    static constexpr float kPostClipLpHz = 7500.0f;
    static constexpr float kPostClipDcBlockHpHz = 20.0f;

    // Small-signal gain ceiling for the raw clipper (pre-filter).
    //
    // 1.15 was calibrated against a tanh curve whose gain fell continuously from
    // 1.95x at silence to 0.92x at full scale — for that shape, small-signal gain
    // was the only handle on how hard the whole tail got squashed. The reciprocal
    // curve is straight below threshold, so its small-signal gain is just the flat
    // makeup trim. 1.40 leaves room for that trim while still rejecting a curve
    // that would boost quiet wet trails the way the pre-voicing Legacy curve did
    // (3.4x). Tail swell is bounded separately and more directly by
    // WetOverdriveDiagnosticsTest's dirty/clean ratio ceiling.
    static constexpr float kSmallSignalMaxGain = 1.40f;
    static constexpr float kSmallSignalTestInput = 0.001f;

    static float clipLegacy (float x) noexcept
    {
        auto scaled = x * kDriveLegacy;

        if (scaled > 0.0f)
            scaled *= kAsymPosLegacy;

        return std::tanh (scaled) / std::tanh (kDriveLegacy);
    }

    static float clipTamedTanh (float x) noexcept
    {
        const auto absX = std::abs (x);
        const auto levelT = absX < kQuietReferenceA ? absX / kQuietReferenceA : 1.0f;
        const auto drive = kDriveQuietA + levelT * (kDriveA - kDriveQuietA);
        const auto makeup = kMakeupQuietA + levelT * (kMakeupA - kMakeupQuietA);
        const auto asym = 1.0f + levelT * (kAsymPosA - 1.0f);

        auto scaled = x * drive;

        if (scaled > 0.0f)
            scaled *= asym;

        return makeup * std::tanh (scaled) / std::tanh (drive);
    }

    static float clipTamedTanhSymmetric (float x) noexcept
    {
        const auto absX = std::abs (x);
        const auto levelT = absX < kQuietReferenceA ? absX / kQuietReferenceA : 1.0f;
        const auto drive = kDriveQuietA + levelT * (kDriveA - kDriveQuietA);
        const auto makeup = kMakeupQuietA + levelT * (kMakeupA - kMakeupQuietA);
        const auto scaled = x * drive;
        return makeup * std::tanh (scaled) / std::tanh (drive);
    }

    static float softKneeAbove (float s, float threshold, float knee) noexcept
    {
        if (s <= threshold)
            return s;

        const auto excess = s - threshold;
        return threshold + std::tanh (excess / knee) * knee;
    }

    static float softKneeBelow (float s, float threshold, float knee) noexcept
    {
        if (s >= threshold)
            return s;

        const auto excess = s - threshold;
        return threshold + std::tanh (excess / knee) * knee;
    }

    static float clipDiodeSoft (float x) noexcept
    {
        const auto s = x * kPreGainB;

        if (s >= 0.0f)
            return kMakeupB * softKneeAbove (s, kPosThresholdB, kKneeB);

        return kMakeupB * softKneeBelow (s, -kNegThresholdB, kKneeB);
    }

    static float clipCubicSoft (float x) noexcept
    {
        const auto s = x * kPreGainC;

        if (s > 1.0f)
            return kMakeupC;

        if (s < -1.0f)
            return -kMakeupC;

        const auto clipped = s - (s * s * s) / 3.0f;
        constexpr auto kNorm = 2.0f / 3.0f;
        return kMakeupC * clipped / kNorm;
    }

    /** Spin/Barr reciprocal soft clip: unity below threshold, 1/x rounding above,
        asymptotic to +/-2 before scaling. Normalising by the drive keeps the
        sub-threshold region at exactly unity gain, so it stays genuinely clean. */
    static float clipBarrReciprocal (float x) noexcept
    {
        auto scaled = x * kDriveBarr;

        if (scaled > 0.0f)
            scaled *= kAsymPosBarr;

        const auto magnitude = std::abs (scaled);
        const auto shaped = magnitude < 1.0f ? magnitude : 2.0f - 1.0f / magnitude;
        const auto signed_ = scaled < 0.0f ? -shaped : shaped;

        return kMakeupBarr * signed_ / kDriveBarr;
    }

    static float clipSample (float x, OverdriveCurve curve) noexcept
    {
        switch (curve)
        {
            case OverdriveCurve::Legacy:         return clipLegacy (x);
            case OverdriveCurve::TamedTanh:      return clipTamedTanh (x);
            case OverdriveCurve::DiodeSoftClip:  return clipDiodeSoft (x);
            case OverdriveCurve::CubicSoftClip:  return clipCubicSoft (x);
            case OverdriveCurve::BarrReciprocal: return clipBarrReciprocal (x);
        }

        return clipBarrReciprocal (x);
    }

    static float asymmetricTanh (float x) noexcept
    {
        return clipSample (x, kActiveCurve);
    }

    static float process (float wet, float distnBlend) noexcept
    {
        const auto driven = asymmetricTanh (wet);
        return wet + distnBlend * (driven - wet);
    }

    static float smallSignalGain (OverdriveCurve curve) noexcept
    {
        const auto input = kSmallSignalTestInput;
        const auto output = clipSample (input, curve);
        return output / input;
    }
};

class OnePoleLowpass
{
public:
    void prepare (double sampleRate, float cutoffHz) noexcept
    {
        const auto omega = 2.0f * 3.14159265358979323846f * cutoffHz / static_cast<float> (sampleRate);
        coef = 1.0f - std::exp (-omega);
    }

    void reset() noexcept
    {
        state = 0.0f;
    }

    float process (float x) noexcept
    {
        state += coef * (x - state);
        return state;
    }

private:
    float coef = 0.0f;
    float state = 0.0f;
};

class OnePoleHighpass
{
public:
    void prepare (double sampleRate, float cutoffHz) noexcept
    {
        const auto omega = 2.0f * 3.14159265358979323846f * cutoffHz / static_cast<float> (sampleRate);
        alpha = std::exp (-omega);
    }

    void reset() noexcept
    {
        prevInput = 0.0f;
        prevOutput = 0.0f;
    }

    float process (float x) noexcept
    {
        const auto y = alpha * (prevOutput + x - prevInput);
        prevInput = x;
        prevOutput = y;
        return y;
    }

private:
    float alpha = 0.0f;
    float prevInput = 0.0f;
    float prevOutput = 0.0f;
};

class WetOverdriveState
{
public:
    void prepare (double sampleRate) noexcept
    {
        preClipHp.prepare (sampleRate, WetOverdrive::kPreClipHpHz);
        preClipLp.prepare (sampleRate, WetOverdrive::kPreClipLpHz);
        postClipLp.prepare (sampleRate, WetOverdrive::kPostClipLpHz);
        postClipDcBlock.prepare (sampleRate, WetOverdrive::kPostClipDcBlockHpHz);
        reset();
    }

    void reset() noexcept
    {
        preClipHp.reset();
        preClipLp.reset();
        postClipLp.reset();
        postClipDcBlock.reset();
    }

    float processFilteredBranch (float wet) noexcept
    {
        auto x = preClipHp.process (wet);
        x = preClipLp.process (x);
        x = WetOverdrive::clipSample (x, WetOverdrive::kActiveCurve);
        x = postClipLp.process (x);
        x = postClipDcBlock.process (x);
        return x;
    }

    float process (float wet, float distnBlend) noexcept
    {
        const auto driven = processFilteredBranch (wet);
        return wet + distnBlend * (driven - wet);
    }

private:
    OnePoleHighpass preClipHp;
    OnePoleLowpass preClipLp;
    OnePoleLowpass postClipLp;
    OnePoleHighpass postClipDcBlock;
};

} // namespace sendbloom
