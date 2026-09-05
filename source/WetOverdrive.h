#pragma once

#include <cmath>
#include <juce_dsp/juce_dsp.h>

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
    // Original-inspired fixed-drive branch. Spin's published reciprocal curve
    // motivated this shape; it does not specify a pedal's analog overdrive.
    // Drive, asymmetry, makeup and filters below are unmeasured voicing choices.
    // In particular, positive/negative small-signal slopes differ here: the
    // asymmetry is not evidence of any specific diode or measured transfer.
    // Preserve the accepted voicing until controlled captures support a change.
    static constexpr OverdriveCurve kActiveCurve = OverdriveCurve::BarrReciprocal;

    static constexpr float kDriveBarr = 3.0f;
    // Chosen positive-side asymmetry; the bare reciprocal is symmetric.
    // No product-specific transfer measurement supports this amount.
    static constexpr float kAsymPosBarr = 1.10f;
    // Flat trim. Bracketed from both sides: the dirty branch has to read as
    // *added* rather than as a level drop (GatedBloomChainTest), while the wet
    // tail must not swell when DISTN opens (WetOverdriveDiagnosticsTest's 1.15
    // ceiling, milestone spec 13.7) — otherwise DISTN doubles as a volume knob.
    // 1.20 puts the 220 Hz tail ratio at ~1.06, inside that window.
    //
    // Each polarity is linear below its knee, but the unequal slopes produce
    // even harmonics across zero, including on quiet trails.
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

    /** Reciprocal soft clip: linear below each knee, 1/x rounding above.
        The bare curve approaches +/-2; asymmetry and makeup change both slopes.
        This is an engineering voicing, not a measured circuit transfer. */
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
    void prepare (double sampleRate, int maxBlockSize = 1) noexcept
    {
        oversampling.initProcessing (static_cast<size_t> (maxBlockSize));
        returned.setSize (2, maxBlockSize);
        const auto processingRate = sampleRate * 4.0;
        preClipHp.prepare (processingRate, WetOverdrive::kPreClipHpHz);
        preClipLp.prepare (processingRate, WetOverdrive::kPreClipLpHz);
        postClipLp.prepare (processingRate, WetOverdrive::kPostClipLpHz);
        postClipDcBlock.prepare (processingRate, WetOverdrive::kPostClipDcBlockHpHz);
        reset();
    }

    int getLatencySamples() const noexcept
    {
        return static_cast<int> (std::lround (oversampling.getLatencyInSamples()));
    }

    void reset() noexcept
    {
        oversampling.reset();
        preClipHp.reset();
        preClipLp.reset();
        postClipLp.reset();
        postClipDcBlock.reset();
    }

    float processFilteredBranch (float wet) noexcept
    {
        float output = 0.0f;
        processBlock (&wet, &output, 1, nullptr, 1.0f);
        return output;
    }

    float process (float wet, float distnBlend) noexcept
    {
        float output = 0.0f;
        processBlock (&wet, &output, 1, nullptr, distnBlend);
        return output;
    }

    void processBlock (const float* wet, float* output, int count,
                       const float* blends, float constantBlend = 0.0f) noexcept
    {
        const float* inputs[] { wet, wet };
        const juce::dsp::AudioBlock<const float> input (inputs, 2, static_cast<size_t> (count));
        auto up = oversampling.processSamplesUp (input);
        auto* dirty = up.getChannelPointer (0);
        for (size_t i = 0; i < up.getNumSamples(); ++i)
            dirty[i] = processAtInternalRate (dirty[i]);
        auto down = juce::dsp::AudioBlock<float> (returned).getSubBlock (0, static_cast<size_t> (count));
        oversampling.processSamplesDown (down);
        for (int i = 0; i < count; ++i)
        {
            const auto clean = returned.getSample (1, i);
            const auto blend = blends != nullptr ? blends[i] : constantBlend;
            output[i] = clean + blend * (returned.getSample (0, i) - clean);
        }
    }

private:
    float processAtInternalRate (float wet) noexcept
    {
        auto x = preClipHp.process (wet);
        x = preClipLp.process (x);
        x = WetOverdrive::clipSample (x, WetOverdrive::kActiveCurve);
        x = postClipLp.process (x);
        x = postClipDcBlock.process (x);
        return x;
    }

    // Both wet branches share reconstruction phase. Integer latency is included
    // in chain PDC and direct-path alignment; never blend a delayed dirty return
    // with an undelayed clean return.
    juce::dsp::Oversampling<float> oversampling {
        2, 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true };
    juce::AudioBuffer<float> returned;
    OnePoleHighpass preClipHp;
    OnePoleLowpass preClipLp;
    OnePoleLowpass postClipLp;
    OnePoleHighpass postClipDcBlock;
};

} // namespace sendbloom
