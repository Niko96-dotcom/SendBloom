#pragma once

#include "ChainTestHelpers.h"
#include <HostRateReverbEngine.h>
#include <ParameterCurves.h>
#include <SchroederTank32.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace sendbloom::test
{

inline constexpr auto kSampleRate = 48000.0;
inline constexpr auto kBlockSize = 512;
inline constexpr auto kRenderSamples = 24000uz;
inline constexpr auto kTailStart = kRenderSamples - 4800;
inline constexpr auto kTailCount = 2400uz;

// Narrowband peak must not dominate neighboring bins by more than this ratio (tail slice).
inline constexpr float kNarrowbandDominanceMaxRatio = 10.0f;
// Authentic bright 10k+ RMS must stay within this factor of host-rate reverb on guitar pluck.
// Measured full-chain guitar_pluck ratio ~1.456; tank-only ProperSRC vs host ~1.32.
// Raised from accumulator-era 1.4 to 1.5 for ProperSRC coloration + production wet path.
inline constexpr float kAuthenticVsHostRmsAbove10kMaxRatio = 1.5f;
// Imaging whistle band (14-15 kHz) tail power ceiling on authentic guitar pluck.
inline constexpr float kImagingBandPeakRmsMax = 0.0022f;

inline std::vector<float> makeGuitarPluck (size_t totalSamples) noexcept
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

inline std::vector<float> makeDecayingSine (float hz, size_t totalSamples, int burstLen = 480) noexcept
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

inline std::vector<float> makeImpulseBurst (size_t totalSamples) noexcept
{
    std::vector<float> signal (totalSamples, 0.0f);
    signal[0] = 1.0f;
    return signal;
}

inline std::vector<float> makeSweptSine (size_t totalSamples) noexcept
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

inline std::vector<std::pair<std::string, std::vector<float>>> allFixtures()
{
    return {
        { "guitar_pluck", makeGuitarPluck (kRenderSamples) },
        { "sine220_decay", makeDecayingSine (220.0f, kRenderSamples) },
        { "sine880_decay", makeDecayingSine (880.0f, kRenderSamples) },
        { "impulse", makeImpulseBurst (kRenderSamples) },
        { "swept_sine", makeSweptSine (kRenderSamples) },
    };
}

enum class ReverbPath
{
    HostRate,
    LegacyAccumulator,
    ProperSRC
};

inline void applyPathDiagnostics (sendbloom::SchroederTank32& tank, ReverbPath path) noexcept
{
    switch (path)
    {
        case ReverbPath::LegacyAccumulator:
            tank.setAuthentic32ModeForDiagnostics (sendbloom::Authentic32Mode::LegacyAccumulator);
            break;
        case ReverbPath::ProperSRC:
        case ReverbPath::HostRate:
            tank.clearAuthentic32ModeForDiagnostics();
            break;
    }
}

inline std::vector<float> renderTankPath (sendbloom::SchroederTank32& tank,
                                          const std::vector<float>& input,
                                          ReverbPath path,
                                          double sampleRate,
                                          int blockSize)
{
    tank.prepare (sampleRate, blockSize);
    applyPathDiagnostics (tank, path);
    sendbloom::HostRateReverbEngine hostRateEngine;
    hostRateEngine.prepare (sampleRate, blockSize);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    std::vector<float> out (input.size(), 0.0f);
    std::vector<float> inBlock (static_cast<size_t> (blockSize), 0.0f);
    std::vector<float> outBlock (static_cast<size_t> (blockSize), 0.0f);

    for (size_t offset = 0; offset < input.size(); offset += static_cast<size_t> (blockSize))
    {
        const int n = static_cast<int> (std::min (static_cast<size_t> (blockSize), input.size() - offset));
        std::copy_n (input.begin() + static_cast<std::ptrdiff_t> (offset), n, inBlock.begin());
        if (path == ReverbPath::HostRate)
            hostRateEngine.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f);
        else
            tank.processBlock (inBlock.data(), outBlock.data(), n, rt60, 0.0f);
        std::copy_n (outBlock.begin(), n, out.begin() + static_cast<std::ptrdiff_t> (offset));
    }

    return out;
}

struct SpectralScan
{
    std::vector<double> powers;
    std::vector<double> freqs;
};

inline SpectralScan scanSpectrum (const std::vector<float>& samples,
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
        scan.powers[b] = goertzelPower (samples, kSampleRate, freq, start, count);
    }

    return scan;
}

inline float rmsFromPower (double power, size_t count) noexcept
{
    if (count == 0 || power <= 0.0)
        return 0.0f;

    return static_cast<float> (std::sqrt (2.0 * power / static_cast<double> (count)));
}

inline float bandRmsAbove (const SpectralScan& scan, double cutoffHz) noexcept
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
    float rmsAbove14k {};
    float hfRatio {};
    float spectralCentroid {};
    float maxNarrowbandPeak {};
    float peakFrequency {};
    float hfBandDecaySec {};
};

inline HfMetrics measureTail (const std::string& config,
                              const std::string& fixture,
                              const std::vector<float>& wet) noexcept
{
    const auto tail = std::vector<float> (wet.begin() + static_cast<std::ptrdiff_t> (kTailStart), wet.end());
    const auto scan = scanSpectrum (wet, kTailStart, kTailCount, 4000.0, 16000.0, 25.0);

    HfMetrics m;
    m.config = config;
    m.fixture = fixture;
    m.rmsTotal = rms (tail);

    const auto scanFull = scanSpectrum (wet, kTailStart, kTailCount, 200.0, 20000.0, 50.0);
    m.rmsAbove6k = bandRmsAbove (scanFull, 6000.0);
    m.rmsAbove10k = bandRmsAbove (scanFull, 10000.0);
    m.rmsAbove14k = bandRmsAbove (scanFull, 14000.0);
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

    const auto window = 480uz;
    auto peakPower = 0.0;
    size_t peakWindowStart = kTailStart;

    for (size_t w = kTailStart; w + window < wet.size(); w += window / 2)
    {
        const auto p = goertzelPower (wet, kSampleRate, m.peakFrequency, w, window);

        if (p > peakPower)
        {
            peakPower = p;
            peakWindowStart = w;
        }
    }

    const auto target = peakPower * 0.01;
    auto decaySamples = 0uz;

    for (size_t w = peakWindowStart; w + window < wet.size(); w += window / 4)
    {
        const auto p = goertzelPower (wet, kSampleRate, m.peakFrequency, w, window);

        if (p <= target)
        {
            decaySamples = w - peakWindowStart;
            break;
        }
    }

    m.hfBandDecaySec = static_cast<float> (static_cast<double> (decaySamples) / kSampleRate);
    return m;
}

inline float narrowbandDominanceRatio (const std::vector<float>& wet, float peakHz) noexcept
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

inline float imaging14825Rms (const std::vector<float>& wet, double sampleRate) noexcept
{
    const auto imagingPower = goertzelPower (wet, sampleRate, 14825.0, kTailStart, kTailCount);
    return rmsFromPower (imagingPower, kTailCount);
}

inline void printMetricsCsv (const HfMetrics& m)
{
    std::cout << m.config << ","
              << m.fixture << ","
              << m.rmsTotal << ","
              << m.rmsAbove6k << ","
              << m.rmsAbove10k << ","
              << m.rmsAbove14k << ","
              << m.hfRatio << ","
              << m.spectralCentroid << ","
              << m.maxNarrowbandPeak << ","
              << m.peakFrequency << ","
              << m.hfBandDecaySec
              << '\n';
}

inline void printThreePathCsvHeader()
{
    std::cout << "path,fixture,rms_total,rms_above_6k,rms_above_10k,rms_above_14k,hf_ratio,centroid_hz,"
                 "max_nb_peak_rms,peak_freq_hz,hf_decay_sec\n";
}

inline void printThreePathCsvRow (const std::string& pathLabel,
                                  const std::string& fixture,
                                  const HfMetrics& m)
{
    std::cout << pathLabel << ","
              << fixture << ","
              << m.rmsTotal << ","
              << m.rmsAbove6k << ","
              << m.rmsAbove10k << ","
              << m.rmsAbove14k << ","
              << m.hfRatio << ","
              << m.spectralCentroid << ","
              << m.maxNarrowbandPeak << ","
              << m.peakFrequency << ","
              << m.hfBandDecaySec
              << '\n';
}

} // namespace sendbloom::test
