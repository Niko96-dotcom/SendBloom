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
    static constexpr float kTankLfoDepthSamples = 16.0f;

    // Host-rate SVF lowpass after 32,768 Hz upsample (removes ~16.384 kHz imaging foldback).
    static constexpr float kAuthenticAntiImageLpHz = 12000.0f;
};

} // namespace sendbloom
