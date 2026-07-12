#include "ChainTestHelpers.h"
#include "FixedRateAdapter.h"
#include "ParameterCurves.h"
#include "ReverbTestHelpers.h"
#include "SchroederTank32DelayTable.h"
#include "SchroederTankCore.h"

#include <Authentic32Mode.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kPredelaySeconds = sendbloom::SchroederTank32DelayTable::kDarkPredelaySeconds;
constexpr auto kOnsetToleranceSeconds = 0.002f; // ±2 ms vs 55 ms target
constexpr auto kStaleBurstThreshold = 0.02f;

float measureOnsetSeconds (const std::vector<float>& samples,
                           double sampleRate,
                           float threshold = 1.0e-4f)
{
    for (size_t i = 0; i < samples.size(); ++i)
    {
        if (std::abs (samples[static_cast<size_t> (i)]) > threshold)
            return static_cast<float> (static_cast<double> (i) / sampleRate);
    }

    return static_cast<float> (samples.size()) / static_cast<float> (sampleRate);
}

float maxAbsInWindow (const std::vector<float>& samples, size_t start, size_t count)
{
    const auto end = std::min (samples.size(), start + count);
    auto peak = 0.0f;

    for (size_t i = start; i < end; ++i)
        peak = std::max (peak, std::abs (samples[i]));

    return peak;
}

float maxAdjacentDelta (const std::vector<float>& samples)
{
    if (samples.size() < 2)
        return 0.0f;

    auto maxDelta = 0.0f;

    for (size_t i = 1; i < samples.size(); ++i)
        maxDelta = std::max (maxDelta, std::abs (samples[i] - samples[i - 1]));

    return maxDelta;
}

std::vector<float> renderCoreSequence (sendbloom::SchroederTankCore& core,
                                       const std::vector<float>& input,
                                       const std::vector<float>& darkMixPerSample,
                                       float rt60)
{
    std::vector<float> out (input.size(), 0.0f);

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto mix = i < darkMixPerSample.size() ? darkMixPerSample[i] : darkMixPerSample.back();
        out[i] = core.processSample (input[i], rt60, mix);
    }

    return out;
}

std::vector<float> renderAdapterImpulseDark (sendbloom::FixedRateAdapter& adapter,
                                             sendbloom::Authentic32Mode mode,
                                             double hostRate,
                                             float darkMix,
                                             float rt60,
                                             int totalSamples,
                                             int blockSize = 512)
{
    adapter.prepare (hostRate, blockSize);

    std::vector<float> out (static_cast<size_t> (totalSamples), 0.0f);
    std::vector<float> inBlock (static_cast<size_t> (blockSize), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (blockSize), 0.0f);

    int globalSample = 0;

    while (globalSample < totalSamples)
    {
        const int n = std::min (blockSize, totalSamples - globalSample);

        std::fill (inBlock.begin(), inBlock.begin() + n, 0.0f);
        if (globalSample == 0)
            inBlock[0] = 1.0f;

        adapter.processBlock (inBlock.data(), outBlock.data(), n, rt60, darkMix, mode);

        for (int i = 0; i < n; ++i)
            out[static_cast<size_t> (globalSample + i)] = outBlock[static_cast<size_t> (i)];

        globalSample += n;
    }

    return out;
}

} // namespace

TEST_CASE ("Bright mode advances predelay line during silence",
           "[v1][contract][predelay][DSP-01]")
{
    // ADR-V1-12 / DSP-01: darkMix≈0 must still clock the line so stale content
    // does not survive a bright silence window.
    constexpr double sampleRate = 48000.0;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto predelaySamples = static_cast<size_t> (std::lround (kPredelaySeconds * sampleRate));
    const auto primeSamples = predelaySamples * 2;
    const auto brightSilenceSamples = predelaySamples * 2;
    const auto captureSamples = predelaySamples + static_cast<size_t> (sampleRate * 0.02);

    std::vector<float> input (primeSamples + brightSilenceSamples + captureSamples, 0.0f);
    std::vector<float> darkMix (input.size(), 0.0f);

    std::fill (darkMix.begin(), darkMix.begin() + static_cast<std::ptrdiff_t> (primeSamples), 1.0f);
    std::fill (input.begin(), input.begin() + static_cast<std::ptrdiff_t> (primeSamples), 1.0f);

    const auto impulseIndex = primeSamples + brightSilenceSamples;
    input[impulseIndex] = 1.0f;
    darkMix[impulseIndex] = 1.0f;

    sendbloom::SchroederTankCore core;
    core.prepare (sampleRate, 512);
    const auto out = renderCoreSequence (core, input, darkMix, rt60);

    const auto preOnsetWindow = static_cast<size_t> (std::lround ((kPredelaySeconds - kOnsetToleranceSeconds) * sampleRate));
    const auto burstPeak = maxAbsInWindow (out,
                                           impulseIndex,
                                           std::min (preOnsetWindow, out.size() - impulseIndex));

    REQUIRE (burstPeak < kStaleBurstThreshold);
}

TEST_CASE ("Dark mode onset is fixed 55 ms after bright immediate onset",
           "[v1][contract][predelay][DSP-02]")
{
    // ADR-V1-12 / DSP-02: fixed 55 ms tap at processing rate; darkMix blends only.
    constexpr double sampleRate = 44100.0;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int numSamples = 8192;

    sendbloom::SchroederTankCore brightCore;
    sendbloom::SchroederTankCore darkCore;
    brightCore.prepare (sampleRate, 512);
    darkCore.prepare (sampleRate, 512);

    const auto brightIr = sendbloom::test::reverb::renderCoreImpulse (brightCore, rt60, 0.0f, numSamples);
    const auto darkIr = sendbloom::test::reverb::renderCoreImpulse (darkCore, rt60, 1.0f, numSamples);

    const auto brightOnset = measureOnsetSeconds (brightIr, sampleRate);
    const auto darkOnset = measureOnsetSeconds (darkIr, sampleRate);
    const auto delta = darkOnset - brightOnset;

    REQUIRE (delta == Catch::Approx (kPredelaySeconds).margin (kOnsetToleranceSeconds));
}

TEST_CASE ("Dark re-enable after bright silence emits no pre-55 ms stale burst",
           "[v1][contract][predelay][DSP-03]")
{
    // ADR-V1-12 / DSP-03: bright → silence > 55 ms → dark must not release frozen line.
    constexpr double sampleRate = 48000.0;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto predelaySamples = static_cast<size_t> (std::lround (kPredelaySeconds * sampleRate));
    const auto brightSilenceSamples = predelaySamples + static_cast<size_t> (sampleRate * 0.01);
    const auto captureSamples = predelaySamples + static_cast<size_t> (sampleRate * 0.05);

    std::vector<float> input (brightSilenceSamples + captureSamples, 0.0f);
    std::vector<float> darkMix (input.size(), 0.0f);

    const auto impulseIndex = brightSilenceSamples;
    input[impulseIndex] = 1.0f;
    std::fill (darkMix.begin() + static_cast<std::ptrdiff_t> (impulseIndex), darkMix.end(), 1.0f);

    sendbloom::SchroederTankCore core;
    core.prepare (sampleRate, 512);

    // Prime stale content while dark, then bright silence long enough to flush if clocking.
    for (size_t i = 0; i < predelaySamples; ++i)
        core.processSample (1.0f, rt60, 1.0f);

    for (size_t i = 0; i < brightSilenceSamples; ++i)
        core.processSample (0.0f, rt60, 0.0f);

    const auto out = renderCoreSequence (core, input, darkMix, rt60);

    const auto preOnsetWindow = static_cast<size_t> (std::lround ((kPredelaySeconds - kOnsetToleranceSeconds) * sampleRate));
    const auto burstPeak = maxAbsInWindow (out, 0, std::min (preOnsetWindow, out.size()));

    REQUIRE (burstPeak < kStaleBurstThreshold);
}

TEST_CASE ("darkMix automation on steady tone stays finite with bounded deltas",
           "[v1][contract][predelay][DSP-04]")
{
    // ADR-V1-12 / DSP-04: automation finite; adjacent-sample |Δ| bounded.
    constexpr double sampleRate = 48000.0;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int numSamples = 48000;
    constexpr float kMaxAdjacentDelta = 0.05f;

    std::vector<float> input (static_cast<size_t> (numSamples), 0.0f);
    std::vector<float> darkMix (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        input[static_cast<size_t> (i)] = 0.25f * std::sin (2.0f * 3.14159265358979323846f * 440.0f
                                                           * static_cast<float> (i)
                                                           / static_cast<float> (sampleRate));
        darkMix[static_cast<size_t> (i)] = static_cast<float> (i) / static_cast<float> (numSamples - 1);
    }

    sendbloom::SchroederTankCore core;
    core.prepare (sampleRate, 512);
    const auto out = renderCoreSequence (core, input, darkMix, rt60);

    for (const auto sample : out)
    {
        REQUIRE (std::isfinite (sample));
        REQUIRE (std::abs (sample) < 8.0f);
    }

    REQUIRE (maxAdjacentDelta (out) <= kMaxAdjacentDelta);
}

TEST_CASE ("Host-rate and ProperSRC dark onset agree in wall-clock time",
           "[v1][contract][predelay][DSP-02][parity]")
{
    // ADR-V1-12: host SchroederTankCore @ 48 kHz vs ProperSRC adapter agree in seconds.
    constexpr double hostRate = 48000.0;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int numSamples = 12000;

    sendbloom::SchroederTankCore hostCore;
    hostCore.prepare (hostRate, 512);
    const auto hostOut = sendbloom::test::reverb::renderCoreImpulse (hostCore, rt60, 1.0f, numSamples);

    sendbloom::FixedRateAdapter adapter;
    const auto properOut = renderAdapterImpulseDark (adapter,
                                                     sendbloom::Authentic32Mode::ProperSRC,
                                                     hostRate,
                                                     1.0f,
                                                     rt60,
                                                     numSamples);

    const auto hostOnset = measureOnsetSeconds (hostOut, hostRate);
    const auto properOnset = measureOnsetSeconds (properOut, hostRate);

    REQUIRE (properOnset == Catch::Approx (hostOnset).margin (kOnsetToleranceSeconds));
    REQUIRE (hostOnset == Catch::Approx (kPredelaySeconds).margin (kOnsetToleranceSeconds * 2.0f));
}
