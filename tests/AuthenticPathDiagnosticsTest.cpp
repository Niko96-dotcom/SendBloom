// Three-path HF diagnostics for SchroederTank32 at 48 kHz (DIAG-01/02/03).
// Metrics: rmsAbove10k/14k (band-limited tail RMS), peakFrequency (dominant Goertzel bin),
// narrowbandDominanceRatio (peak vs neighbor bins), spectralCentroid (power-weighted Hz),
// imaging14825Rms (14825 Hz imaging whistle — see REQUIREMENTS DIAG-03 / SRC-06).

#include "ChainTestHelpers.h"
#include "HfDiagnosticsHelpers.h"
#include <SchroederTank32.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <string>

namespace
{

using sendbloom::test::ReverbPath;
using sendbloom::test::allFixtures;
using sendbloom::test::imaging14825Rms;
using sendbloom::test::kBlockSize;
using sendbloom::test::kSampleRate;
using sendbloom::test::measureTail;
using sendbloom::test::narrowbandDominanceRatio;
using sendbloom::test::printThreePathCsvHeader;
using sendbloom::test::printThreePathCsvRow;
using sendbloom::test::goertzelPower;
using sendbloom::test::kRenderSamples;
using sendbloom::test::makeGuitarPluck;
using sendbloom::test::renderTankPath;
using sendbloom::test::rms;

// DIAG-04 multi-rate invariance tolerances (15-RESEARCH.md Multi-Rate Tolerance Strategy).
constexpr float kImagingBandPeakRmsMax = 0.0022f;
constexpr float kNarrowbandDominanceMaxRatio = 10.0f;
constexpr float kDiag04RmsRatioMin = 0.65f;
constexpr float kDiag04RmsRatioMax = 1.55f;
constexpr float kDiag04RmsTotalRatioMin = 0.5f;
constexpr float kDiag04RmsTotalRatioMax = 2.0f;
constexpr float kDiag04PeakFreqDeltaMaxHz = 1500.0f;
constexpr float kDiag04CentroidDeltaMaxHz = 3000.0f;
constexpr float kDiag04CrossRateRms10kMaxRatio = 1.6f;

struct RateHfSnapshot
{
    float rmsTotal {};
    float rmsAbove10k {};
    float rmsAbove14k {};
    float peakFrequency {};
    float spectralCentroid {};
};

size_t renderSamplesAtRate (double rate) noexcept
{
    return static_cast<size_t> (std::lround (static_cast<double> (kRenderSamples) * rate / kSampleRate));
}

std::pair<size_t, size_t> tailWindowForRate (double rate, size_t wetSize) noexcept
{
    constexpr auto kTailLeadSec = 4800.0 / kSampleRate;
    constexpr auto kTailWindowSec = 2400.0 / kSampleRate;
    const auto tailCount = static_cast<size_t> (std::lround (rate * kTailWindowSec));
    const auto tailLead = static_cast<size_t> (std::lround (rate * kTailLeadSec));
    const auto tailStart = wetSize >= tailLead ? wetSize - tailLead : 0uz;
    return { tailStart, tailCount };
}

float rmsFromPowerAtCount (double power, size_t count) noexcept
{
    if (count == 0 || power <= 0.0)
        return 0.0f;

    return static_cast<float> (std::sqrt (2.0 * power / static_cast<double> (count)));
}

float bandRmsAboveAtRate (const std::vector<float>& wet,
                          double rate,
                          size_t tailStart,
                          size_t tailCount,
                          double cutoffHz) noexcept
{
    double sum = 0.0;

    for (double freq = 200.0; freq <= 20000.0; freq += 50.0)
    {
        if (freq >= cutoffHz)
            sum += goertzelPower (wet, rate, freq, tailStart, tailCount);
    }

    return rmsFromPowerAtCount (sum, 1);
}

RateHfSnapshot measureRateTail (const std::vector<float>& wet, double rate) noexcept
{
    const auto [tailStart, tailCount] = tailWindowForRate (rate, wet.size());
    const auto tail = std::vector<float> (wet.begin() + static_cast<std::ptrdiff_t> (tailStart), wet.end());

    RateHfSnapshot snap;
    snap.rmsTotal = rms (tail);
    snap.rmsAbove10k = bandRmsAboveAtRate (wet, rate, tailStart, tailCount, 10000.0);
    snap.rmsAbove14k = bandRmsAboveAtRate (wet, rate, tailStart, tailCount, 14000.0);

    double weighted = 0.0;
    double totalPower = 0.0;
    double peakPower = 0.0;
    float peakFreq = 4000.0f;

    for (double freq = 4000.0; freq <= 16000.0; freq += 25.0)
    {
        const auto p = goertzelPower (wet, rate, freq, tailStart, tailCount);
        weighted += freq * p;
        totalPower += p;

        if (p > peakPower)
        {
            peakPower = p;
            peakFreq = static_cast<float> (freq);
        }
    }

    snap.peakFrequency = peakFreq;
    snap.spectralCentroid = totalPower > 1e-12 ? static_cast<float> (weighted / totalPower) : 0.0f;
    return snap;
}

float narrowbandDominanceAtRate (const std::vector<float>& wet,
                                 double rate,
                                 float peakHz,
                                 size_t tailStart,
                                 size_t tailCount) noexcept
{
    double peakPower = 0.0;
    double neighborSum = 0.0;
    int neighborCount = 0;

    for (double freq = peakHz - 500.0; freq <= peakHz + 500.0; freq += 25.0)
    {
        const auto p = goertzelPower (wet, rate, freq, tailStart, tailCount);
        peakPower = std::max (peakPower, p);

        const auto df = std::abs (freq - static_cast<double> (peakHz));

        if (df >= 75.0 && df <= 200.0)
        {
            neighborSum += p;
            ++neighborCount;
        }
    }

    const auto neighborMean = neighborCount > 0 ? neighborSum / static_cast<double> (neighborCount) : 1e-12;
    return static_cast<float> (peakPower / std::max (neighborMean, 1e-12));
}

std::vector<float> renderProperSrcAtRate (const std::vector<float>& input,
                                          double rate,
                                          int blockSize) noexcept
{
    sendbloom::SchroederTank32 tank;
    return renderTankPath (tank, input, ReverbPath::ProperSRC, rate, blockSize);
}

float imaging14825RmsAtRate (const std::vector<float>& wet, double rate) noexcept
{
    const auto [tailStart, tailCount] = tailWindowForRate (rate, wet.size());
    const auto imagingPower = goertzelPower (wet, rate, 14825.0, tailStart, tailCount);
    return rmsFromPowerAtCount (imagingPower, tailCount);
}

std::vector<float> guitarPluckAtRate (double rate)
{
    return makeGuitarPluck (renderSamplesAtRate (rate));
}

const char* pathLabel (ReverbPath path) noexcept
{
    switch (path)
    {
        case ReverbPath::HostRate: return "HostRate";
        case ReverbPath::LegacyAccumulator: return "LegacyAccumulator";
        case ReverbPath::ProperSRC: return "ProperSRC";
    }

    return "unknown";
}

std::vector<float> renderFreshTankPath (const std::vector<float>& input, ReverbPath path)
{
    sendbloom::SchroederTank32 tank;
    return renderTankPath (tank, input, path, kSampleRate, kBlockSize);
}

} // namespace

TEST_CASE ("Three-path render matrix CSV at 48 kHz", "[diagnostics][DIAG-01]")
{
    const ReverbPath paths[] {
        ReverbPath::HostRate,
        ReverbPath::LegacyAccumulator,
        ReverbPath::ProperSRC,
    };

    printThreePathCsvHeader();

    float legacyGuitarImaging = 0.0f;
    float properGuitarImaging = 0.0f;

    for (const auto& [fixtureName, input] : allFixtures())
    {
        for (const auto path : paths)
        {
            const auto wet = renderFreshTankPath (input, path);
            const auto m = measureTail (pathLabel (path), fixtureName, wet);
            printThreePathCsvRow (pathLabel (path), fixtureName, m);

            for (const auto s : wet)
                REQUIRE (std::isfinite (s));

            if (fixtureName == "guitar_pluck")
            {
                const auto imaging = imaging14825Rms (wet, kSampleRate);

                if (path == ReverbPath::LegacyAccumulator)
                    legacyGuitarImaging = imaging;
                else if (path == ReverbPath::ProperSRC)
                    properGuitarImaging = imaging;
            }
        }
    }

    INFO ("legacy guitar 14825 Hz RMS = " << legacyGuitarImaging);
    INFO ("proper guitar 14825 Hz RMS = " << properGuitarImaging);
    REQUIRE (legacyGuitarImaging > properGuitarImaging);
    REQUIRE (properGuitarImaging < kImagingBandPeakRmsMax);

    SUCCEED ("three-path matrix metrics printed to stdout");
}

TEST_CASE ("Five fixtures render finite across three-path harness", "[diagnostics][DIAG-02]")
{
    const auto fixtures = allFixtures();
    REQUIRE (fixtures.size() == 5);

    const std::set<std::string> expected {
        "guitar_pluck",
        "sine220_decay",
        "sine880_decay",
        "impulse",
        "swept_sine",
    };

    for (const auto& [name, input] : fixtures)
        REQUIRE (expected.count (name) == 1);

    for (const auto& [fixtureName, input] : fixtures)
    {
        const auto properWet = renderFreshTankPath (input, ReverbPath::ProperSRC);

        for (const auto s : properWet)
            REQUIRE (std::isfinite (s));
    }

    const auto impulseIt = std::find_if (fixtures.begin(),
                                       fixtures.end(),
                                       [] (const auto& p) { return p.first == "impulse"; });
    REQUIRE (impulseIt != fixtures.end());

    for (const auto path : { ReverbPath::HostRate, ReverbPath::LegacyAccumulator })
    {
        const auto wet = renderFreshTankPath (impulseIt->second, path);

        for (const auto s : wet)
            REQUIRE (std::isfinite (s));
    }
}

TEST_CASE ("ProperSRC HF metric suite on guitar pluck tail", "[diagnostics][DIAG-03]")
{
    const auto fixtures = allFixtures();
    const auto guitarIt = std::find_if (fixtures.begin(),
                                        fixtures.end(),
                                        [] (const auto& p) { return p.first == "guitar_pluck"; });
    REQUIRE (guitarIt != fixtures.end());

    const auto& input = guitarIt->second;

    const auto properWet = renderFreshTankPath (input, ReverbPath::ProperSRC);
    const auto legacyWet = renderFreshTankPath (input, ReverbPath::LegacyAccumulator);
    renderFreshTankPath (input, ReverbPath::HostRate);

    const auto m = measureTail ("ProperSRC", "guitar_pluck", properWet);
    // Probe imaging-band dominance (SRC-06 / DIAG-04), not global tail peak — a mid-band
    // reverb peak can exceed 10:1 without being the 14–15 kHz glass whistle.
    constexpr float kImagingProbeHz = 14825.0f;
    const auto dominance = narrowbandDominanceRatio (properWet, kImagingProbeHz);
    const auto properImaging = imaging14825Rms (properWet, kSampleRate);
    const auto legacyImaging = imaging14825Rms (legacyWet, kSampleRate);

    REQUIRE (m.rmsAbove10k > 0.0f);
    REQUIRE (m.rmsAbove14k >= 0.0f);
    REQUIRE (m.peakFrequency >= 4000.0f);
    REQUIRE (m.peakFrequency <= 16000.0f);
    REQUIRE (m.spectralCentroid > 0.0f);
    REQUIRE (dominance < kNarrowbandDominanceMaxRatio);
    REQUIRE (properImaging < kImagingBandPeakRmsMax);
    REQUIRE (legacyImaging > properImaging);

    INFO ("proper imaging14825 RMS = " << properImaging);
    INFO ("legacy imaging14825 RMS = " << legacyImaging);
    INFO ("peak freq = " << m.peakFrequency << " Hz dominance = " << dominance);
}

TEST_CASE ("ProperSRC HF metrics invariant across host rates", "[diagnostics][DIAG-04]")
{
    constexpr std::array<double, 3> kDiag04Rates { 44100.0, 48000.0, 96000.0 };
    constexpr double kReferenceRate = 48000.0;

    const auto referenceInput = guitarPluckAtRate (kReferenceRate);
    const auto referenceWet = renderProperSrcAtRate (referenceInput, kReferenceRate, kBlockSize);
    const auto reference = measureRateTail (referenceWet, kReferenceRate);
    const auto [refTailStart, refTailCount] = tailWindowForRate (kReferenceRate, referenceWet.size());
    // Narrowband dominance probed at 14825 Hz imaging band (SRC-06 / DIAG-03); global tail peak
    // shifts to mid-band at 96 kHz and is not an HF whistle discriminator across host rates.
    constexpr float kImagingProbeHz = 14825.0f;
    const auto refDominance = narrowbandDominanceAtRate (referenceWet,
                                                         kReferenceRate,
                                                         kImagingProbeHz,
                                                         refTailStart,
                                                         refTailCount);
    const auto refImaging = imaging14825RmsAtRate (referenceWet, kReferenceRate);

    REQUIRE (refImaging < kImagingBandPeakRmsMax);
    REQUIRE (refDominance < kNarrowbandDominanceMaxRatio);

    std::vector<float> rmsAbove10kByRate;

    for (const auto rate : kDiag04Rates)
    {
        const auto input = guitarPluckAtRate (rate);
        const auto wet = renderProperSrcAtRate (input, rate, kBlockSize);
        const auto snap = measureRateTail (wet, rate);
        const auto [tailStart, tailCount] = tailWindowForRate (rate, wet.size());
        const auto dominance = narrowbandDominanceAtRate (wet,
                                                         rate,
                                                         kImagingProbeHz,
                                                         tailStart,
                                                         tailCount);
        const auto imaging = imaging14825RmsAtRate (wet, rate);

        REQUIRE (imaging < kImagingBandPeakRmsMax);
        REQUIRE (dominance < kNarrowbandDominanceMaxRatio);

        rmsAbove10kByRate.push_back (snap.rmsAbove10k);

        if (std::abs (rate - kReferenceRate) < 0.5)
            continue;

        const auto rms10kRatio = snap.rmsAbove10k / std::max (reference.rmsAbove10k, 1e-9f);
        const auto rms14kRatio = snap.rmsAbove14k / std::max (reference.rmsAbove14k, 1e-9f);
        const auto rmsTotalRatio = snap.rmsTotal / std::max (reference.rmsTotal, 1e-9f);

        INFO ("rate = " << rate << " rms10k ratio = " << rms10kRatio);
        INFO ("rate = " << rate << " rms14k ratio = " << rms14kRatio);
        INFO ("rate = " << rate << " peak delta = " << std::abs (snap.peakFrequency - reference.peakFrequency));
        INFO ("rate = " << rate << " centroid delta = " << std::abs (snap.spectralCentroid - reference.spectralCentroid));

        REQUIRE (rms10kRatio >= kDiag04RmsRatioMin);
        REQUIRE (rms10kRatio <= kDiag04RmsRatioMax);
        REQUIRE (rms14kRatio >= kDiag04RmsRatioMin);
        REQUIRE (rms14kRatio <= kDiag04RmsRatioMax);
        REQUIRE (rmsTotalRatio >= kDiag04RmsTotalRatioMin);
        REQUIRE (rmsTotalRatio <= kDiag04RmsTotalRatioMax);
        REQUIRE (std::abs (snap.peakFrequency - reference.peakFrequency) <= kDiag04PeakFreqDeltaMaxHz);
        REQUIRE (std::abs (snap.spectralCentroid - reference.spectralCentroid) <= kDiag04CentroidDeltaMaxHz);
    }

    const auto minRms10k = *std::min_element (rmsAbove10kByRate.begin(), rmsAbove10kByRate.end());
    const auto maxRms10k = *std::max_element (rmsAbove10kByRate.begin(), rmsAbove10kByRate.end());
    const auto crossRateRatio = maxRms10k / std::max (minRms10k, 1e-9f);

    INFO ("cross-rate rmsAbove10k max/min = " << crossRateRatio);
    REQUIRE (crossRateRatio <= kDiag04CrossRateRms10kMaxRatio);
}

TEST_CASE ("ProperSRC output finite for all fixtures and host rates", "[diagnostics][TEST-08]")
{
    constexpr std::array<double, 4> kHostRates { 44100.0, 48000.0, 88200.0, 96000.0 };

    for (const auto rate : kHostRates)
    {
        for (const auto& [fixtureName, input] : allFixtures())
        {
            const auto wet = renderProperSrcAtRate (input, rate, kBlockSize);

            for (const auto s : wet)
                REQUIRE (std::isfinite (s));
        }
    }
}

TEST_CASE ("ProperSRC output finite across variable block sizes", "[diagnostics][TEST-08][blockSize]")
{
    const auto fixtures = allFixtures();
    const auto guitarIt = std::find_if (fixtures.begin(),
                                        fixtures.end(),
                                        [] (const auto& p) { return p.first == "guitar_pluck"; });
    REQUIRE (guitarIt != fixtures.end());

    constexpr std::array<int, 3> kBlockSizes { 1, 64, 512 };

    for (const auto blockSize : kBlockSizes)
    {
        const auto wet = renderProperSrcAtRate (guitarIt->second, kSampleRate, blockSize);

        for (const auto s : wet)
            REQUIRE (std::isfinite (s));
    }
}
