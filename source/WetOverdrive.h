#pragma once

#include <cmath>

namespace sendbloom
{

enum class OverdriveCurve
{
    Legacy = 0,
    TamedTanh = 1,
    DiodeSoftClip = 2,
    CubicSoftClip = 3
};

struct WetOverdrive
{
    static constexpr OverdriveCurve kActiveCurve = OverdriveCurve::TamedTanh;

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

    // Small-signal gain ceiling for the raw clipper (pre-filter).
    static constexpr float kSmallSignalMaxGain = 1.15f;
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

    static float clipSample (float x, OverdriveCurve curve) noexcept
    {
        switch (curve)
        {
            case OverdriveCurve::Legacy:        return clipLegacy (x);
            case OverdriveCurve::TamedTanh:     return clipTamedTanh (x);
            case OverdriveCurve::DiodeSoftClip: return clipDiodeSoft (x);
            case OverdriveCurve::CubicSoftClip: return clipCubicSoft (x);
        }

        return clipTamedTanh (x);
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

class WetOverdriveState
{
public:
    void prepare (double sampleRate) noexcept
    {
        preClipLp.prepare (sampleRate, WetOverdrive::kPreClipLpHz);
        postClipLp.prepare (sampleRate, WetOverdrive::kPostClipLpHz);
        reset();
    }

    void reset() noexcept
    {
        preClipLp.reset();
        postClipLp.reset();
    }

    float processFilteredBranch (float wet) noexcept
    {
        const auto bandLimited = preClipLp.process (wet);
        const auto clipped = WetOverdrive::clipSample (bandLimited, WetOverdrive::kActiveCurve);
        return postClipLp.process (clipped);
    }

    float process (float wet, float distnBlend) noexcept
    {
        const auto driven = processFilteredBranch (wet);
        return wet + distnBlend * (driven - wet);
    }

private:
    OnePoleLowpass preClipLp;
    OnePoleLowpass postClipLp;
};

} // namespace sendbloom
