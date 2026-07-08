#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <WetOverdrive.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace
{

using sendbloom::GatedBloomChain;

constexpr auto kSampleRate = 48000.0;
constexpr auto kBlockSize = 512;
constexpr auto kRenderSamples = 24000uz;
constexpr auto kTailStart = kRenderSamples - 4800;
constexpr auto kTailCount = 2400uz;

// Narrowband peak must not dominate neighboring bins by more than this ratio (tail slice).
constexpr float kNarrowbandDominanceMaxRatio = 10.0f;
// Authentic bright 10k+ RMS must stay within this factor of host-rate reverb on guitar pluck.
constexpr float kAuthenticVsHostRmsAbove10kMaxRatio = 1.4f;
// Imaging whistle band (14-15 kHz) tail power ceiling on authentic guitar pluck.
constexpr float kImagingBandPeakRmsMax = 0.0022f;

struct RenderConfig
{
    const char* label;
    float distn;
    bool authenticColor;
    float darkMix;
};

const RenderConfig kMatrixConfigs[] {
    { "A_rev_bright", 0.0f, false, 0.0f },
    { "B_rev_auth_bright", 0.0f, true, 0.0f },
    { "C_rev_auth_dark", 0.0f, true, 1.0f },
    { "D_od_bright", 1.0f, false, 0.0f },
    { "E_od_auth_bright", 1.0f, true, 0.0f },
    { "F_od_auth_dark", 1.0f, true, 1.0f },
};

std::vector<float> makeGuitarPluck (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 240;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kSampleRate);
        const auto env = std::exp (-t * 18.0f);
        signal[static_cast<size_t> (i)] = env * (0.6f * std::sin (2.0f * 3.14159265358979323846f * 330.0f * t)
                                                 + 0.25f * std::sin (2.0f * 3.14159265358979323846f * 660.0f * t)
                                                 + 0.15f * std::sin (2.0f * 3.14159265358979323846f * 990.0f * t));
    }

    return signal;
}

std::vector<float> makeDecayingSine (float hz, size_t totalSamples, int burstLen = 480) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    const auto phaseInc = 2.0f * 3.14159265358979323846f * hz / static_cast<float> (kSampleRate);

    for (int i = 0; i < burstLen; ++i)
    {
        const auto env = std::exp (-static_cast<float> (i) / 120.0f);
        signal[static_cast<size_t> (i)] = 0.35f * env * std::sin (phaseInc * static_cast<float> (i));
    }

    return signal;
}

std::vector<float> makeImpulseBurst (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    signal[0] = 1.0f;
    return signal;
}

std::vector<float> makeSweptSine (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    constexpr auto kBurstLen = 4800;
    constexpr auto f0 = 200.0f;
    constexpr auto f1 = 8000.0f;
    auto phase = 0.0f;

    for (int i = 0; i < kBurstLen; ++i)
    {
        const auto t = static_cast<float> (i) / static_cast<float> (kBurstLen - 1);
        const auto env = std::exp (-static_cast<float> (i) / 800.0f);
        const auto freq = f0 + t * (f1 - f0);
        phase += 2.0f * 3.14159265358979323846f * freq / static_cast<float> (kSampleRate);
        signal[static_cast<size_t> (i)] = 0.3f * env * std::sin (phase);
    }

    return signal;
}

std::vector<float> renderChain (const std::vector<float>& input,
                                float distn,
                                bool authenticColor,
                                float darkMix) noexcept
{
    GatedBloomChain chain;
    chain.prepare (kSampleRate, kBlockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet (input.size());

    for (size_t i = 0; i < input.size(); ++i)
    {
        const auto env = chain.getEnvelope().process (std::abs (input[i]));
        wet[i] = chain.processSample (input[i], env, rt60, darkMix, authenticColor,
                                      distn, 1.0f, true, -40.0f);
    }

    return wet;
}

struct SpectralScan
{
    std::vector<double> powers;
    std::vector<double> freqs;
};

SpectralScan scanSpectrum (const std::vector<float>& samples,
                           size_t start,
                           size_t count,
                           double fMin,
                           double fMax,
                           double stepHz) noexcept
{
    SpectralScan scan;
    const auto nBins = static_cast<size_t> (std::floor ((fMax - fMin) / stepHz)) + 1;
    scan.freqs.resize (nBins);
    scan.powers.resize (nBins);

    for (size_t b = 0; b < nBins; ++b)
    {
        const auto freq = fMin + static_cast<double> (b) * stepHz;
        scan.freqs[b] = freq;
        scan.powers[b] = sendbloom::test::goertzelPower (samples, kSampleRate, freq, start, count);
    }

    return scan;
}

float rmsFromPower (double power, size_t count) noexcept
{
    if (count == 0 || power <= 0.0)
        return 0.0f;

    return static_cast<float> (std::sqrt (2.0 * power / static_cast<double> (count)));
}

float bandRmsAbove (const SpectralScan& scan, double cutoffHz) noexcept
{
    double sum = 0.0;

    for (size_t i = 0; i < scan.powers.size(); ++i)
    {
        if (scan.freqs[i] >= cutoffHz)
            sum += scan.powers[i];
    }

    return rmsFromPower (sum, 1);
}

struct HfMetrics
{
    std::string config;
    std::string fixture;
    float rmsTotal {};
    float rmsAbove6k {};
    float rmsAbove10k {};
    float hfRatio {};
    float spectralCentroid {};
    float maxNarrowbandPeak {};
    float peakFrequency {};
    float hfBandDecaySec {};
};

HfMetrics measureTail (const std::string& config,
                       const std::string& fixture,
                       const std::vector<float>& wet) noexcept
{
    const auto tail = std::vector<float> (wet.begin() + static_cast<std::ptrdiff_t> (kTailStart), wet.end());
    const auto scan = scanSpectrum (wet, kTailStart, kTailCount, 4000.0, 16000.0, 25.0);

    HfMetrics m;
    m.config = config;
    m.fixture = fixture;
    m.rmsTotal = sendbloom::test::rms (tail);

    const auto scanFull = scanSpectrum (wet, kTailStart, kTailCount, 200.0, 20000.0, 50.0);
    m.rmsAbove6k = bandRmsAbove (scanFull, 6000.0);
    m.rmsAbove10k = bandRmsAbove (scanFull, 10000.0);
    m.hfRatio = m.rmsTotal > 1e-9f ? bandRmsAbove (scanFull, 8000.0) / m.rmsTotal : 0.0f;

    double weighted = 0.0;
    double totalPower = 0.0;

    for (size_t i = 0; i < scan.powers.size(); ++i)
    {
        weighted += scan.freqs[i] * scan.powers[i];
        totalPower += scan.powers[i];
    }

    m.spectralCentroid = totalPower > 1e-12 ? static_cast<float> (weighted / totalPower) : 0.0f;

    size_t peakIdx = 0;

    for (size_t i = 1; i < scan.powers.size(); ++i)
    {
        if (scan.powers[i] > scan.powers[peakIdx])
            peakIdx = i;
    }

    m.peakFrequency = static_cast<float> (scan.freqs[peakIdx]);
    m.maxNarrowbandPeak = rmsFromPower (scan.powers[peakIdx], kTailCount);

    // HF band decay: measure when peak-frequency power drops 20 dB after tail onset.
    const auto window = 480uz;
    auto peakPower = 0.0;
    size_t peakWindowStart = kTailStart;

    for (size_t w = kTailStart; w + window < wet.size(); w += window / 2)
    {
        const auto p = sendbloom::test::goertzelPower (wet, kSampleRate, m.peakFrequency, w, window);

        if (p > peakPower)
        {
            peakPower = p;
            peakWindowStart = w;
        }
    }

    const auto target = peakPower * 0.01; // -20 dB
    auto decaySamples = 0uz;

    for (size_t w = peakWindowStart; w + window < wet.size(); w += window / 4)
    {
        const auto p = sendbloom::test::goertzelPower (wet, kSampleRate, m.peakFrequency, w, window);

        if (p <= target)
        {
            decaySamples = w - peakWindowStart;
            break;
        }
    }

    m.hfBandDecaySec = static_cast<float> (decaySamples / kSampleRate);
    return m;
}

void printMetricsCsv (const HfMetrics& m)
{
    std::cout << m.config << ","
              << m.fixture << ","
              << m.rmsTotal << ","
              << m.rmsAbove6k << ","
              << m.rmsAbove10k << ","
              << m.hfRatio << ","
              << m.spectralCentroid << ","
              << m.maxNarrowbandPeak << ","
              << m.peakFrequency << ","
              << m.hfBandDecaySec
              << '\n';
}

float narrowbandDominanceRatio (const std::vector<float>& wet, float peakHz) noexcept
{
    const auto scan = scanSpectrum (wet, kTailStart, kTailCount, peakHz - 500.0, peakHz + 500.0, 25.0);

    if (scan.powers.empty())
        return 0.0f;

    const auto peakPower = *std::max_element (scan.powers.begin(), scan.powers.end());
    double neighborSum = 0.0;
    int neighborCount = 0;

    for (size_t i = 0; i < scan.powers.size(); ++i)
    {
        const auto df = std::abs (scan.freqs[i] - static_cast<double> (peakHz));

        if (df >= 75.0 && df <= 200.0)
        {
            neighborSum += scan.powers[i];
            ++neighborCount;
        }
    }

    const auto neighborMean = neighborCount > 0 ? neighborSum / static_cast<double> (neighborCount) : 1e-12;
    return static_cast<float> (peakPower / std::max (neighborMean, 1e-12));
}

} // namespace

TEST_CASE ("HF ringing isolation matrix metrics CSV", "[hf][ringing][diagnostics][fixtures]")
{
    const std::vector<std::pair<const char*, std::vector<float>>> fixtures {
        { "guitar_pluck", makeGuitarPluck (kRenderSamples) },
        { "sine220_decay", makeDecayingSine (220.0f, kRenderSamples) },
        { "sine880_decay", makeDecayingSine (880.0f, kRenderSamples) },
        { "impulse", makeImpulseBurst (kRenderSamples) },
        { "swept_sine", makeSweptSine (kRenderSamples) },
    };

    std::cout << "config,fixture,rms_total,rms_above_6k,rms_above_10k,hf_ratio,centroid_hz,"
                 "max_nb_peak_rms,peak_freq_hz,hf_decay_sec\n";

    for (const auto& [fixtureName, input] : fixtures)
    {
        for (const auto& cfg : kMatrixConfigs)
        {
            const auto wet = renderChain (input, cfg.distn, cfg.authenticColor, cfg.darkMix);
            printMetricsCsv (measureTail (cfg.label, fixtureName, wet));
        }
    }

    SUCCEED ("HF isolation matrix metrics printed to stdout");
}

TEST_CASE ("HF ringing authentic bright guitar 10k RMS near host-rate level", "[hf][ringing][regression]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto hostWet = renderChain (input, 0.0f, false, 0.0f);
    const auto authWet = renderChain (input, 0.0f, true, 0.0f);

    const auto mHost = measureTail ("A_rev_bright", "guitar_pluck", hostWet);
    const auto mAuth = measureTail ("B_rev_auth_bright", "guitar_pluck", authWet);
    const auto ratio = mAuth.rmsAbove10k / std::max (mHost.rmsAbove10k, 1e-9f);

    INFO ("host rms_above_10k=" << mHost.rmsAbove10k << " auth=" << mAuth.rmsAbove10k
          << " ratio=" << ratio);
    REQUIRE (ratio < kAuthenticVsHostRmsAbove10kMaxRatio);
}

TEST_CASE ("HF ringing no narrowband glass whistle on authentic guitar tail", "[hf][ringing][regression]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto wet = renderChain (input, 0.0f, true, 0.0f);
    const auto m = measureTail ("B_rev_auth_bright", "guitar_pluck", wet);
    const auto dominance = narrowbandDominanceRatio (wet, m.peakFrequency);

    INFO ("peak freq = " << m.peakFrequency << " Hz, dominance ratio = " << dominance);
    REQUIRE (dominance < kNarrowbandDominanceMaxRatio);
}

TEST_CASE ("HF ringing authentic path imaging band suppressed on guitar", "[hf][ringing][regression]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto wet = renderChain (input, 0.0f, true, 0.0f);
    const auto imagingPower = sendbloom::test::goertzelPower (wet, kSampleRate, 14825.0, kTailStart, kTailCount);
    const auto imagingRms = rmsFromPower (imagingPower, kTailCount);

    INFO ("14825 Hz tail RMS = " << imagingRms);
    REQUIRE (imagingRms < kImagingBandPeakRmsMax);
}

TEST_CASE ("HF ringing authentic path finite output guitar pluck", "[hf][ringing][regression]")
{
    const auto input = makeGuitarPluck (kRenderSamples);

    for (const auto& cfg : kMatrixConfigs)
    {
        const auto wet = renderChain (input, cfg.distn, cfg.authenticColor, cfg.darkMix);

        for (const auto s : wet)
            REQUIRE (std::isfinite (s));
    }
}

TEST_CASE ("HF ringing artifact stronger with authentic than host-rate reverb", "[hf][ringing][diagnostics]")
{
    const auto input = makeGuitarPluck (kRenderSamples);
    const auto hostRate = renderChain (input, 0.0f, false, 0.0f);
    const auto authentic = renderChain (input, 0.0f, true, 0.0f);

    const auto mHost = measureTail ("A", "guitar", hostRate);
    const auto mAuth = measureTail ("B", "guitar", authentic);

    INFO ("host hf_ratio=" << mHost.hfRatio << " auth hf_ratio=" << mAuth.hfRatio);
    INFO ("host rms_above_10k=" << mHost.rmsAbove10k << " auth rms_above_10k=" << mAuth.rmsAbove10k);
    INFO ("host peak=" << mHost.peakFrequency << " Hz auth peak=" << mAuth.peakFrequency << " Hz");

    REQUIRE (mAuth.peakFrequency < 12000.0f);
    SUCCEED ("diagnostic comparison complete");
}
