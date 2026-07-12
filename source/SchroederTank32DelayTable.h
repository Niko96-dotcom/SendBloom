#pragma once

#include <array>
#include <cstddef>

namespace sendbloom
{

struct SchroederTank32DelayTable
{
    static constexpr double kInternalRate = 32768.0;

    // Coprime lengths @ 32.768 kHz (scaled from Freeverb ratios; see 05-RESEARCH.md)
    static constexpr std::array<int, 4> kSeriesApfDelays { 167, 253, 328, 413 };
    static constexpr std::array<int, 4> kParallelCombDelays { 1057, 1108, 1157, 1202 };
    static constexpr int kTankApDelay = 677;

    static constexpr float kApfFeedback = 0.7f;
    static constexpr float kTankApFeedback = 0.5f;

    static constexpr float kDarkPredelaySeconds = 0.055f;
    static constexpr float kBrightDampingHz = 8000.0f;
    static constexpr float kAuthenticBrightDampingHz = 6500.0f;
    static constexpr float kDarkDampingHz = 3200.0f;

    static constexpr float kTankLfoHz = 0.55f;

    // ADR-V1-13: time-invariant modulation depth (16 samples @ 32.768 kHz).
    static constexpr double kTankLfoDepthSeconds = 16.0 / 32768.0;
    static constexpr float kTankLfoDepthSamples =
        static_cast<float> (kTankLfoDepthSeconds * kInternalRate);

    static float tankLfoDepthSamplesForRate (double rate) noexcept
    {
        return static_cast<float> (kTankLfoDepthSeconds * rate);
    }

    // LegacyAccumulatorPath-only: host-rate SVF lowpass after 32,768 Hz upsample
    // (removes ~16.384 kHz imaging foldback in legacy accumulator tier).
    static constexpr float kAuthenticAntiImageLpHz = 12000.0f;
};

} // namespace sendbloom
