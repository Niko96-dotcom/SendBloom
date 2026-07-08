// Three-path HF diagnostics for SchroederTank32 at 48 kHz (DIAG-01/02/03).
// Metrics: rmsAbove10k/14k (band-limited tail RMS), peakFrequency (dominant Goertzel bin),
// narrowbandDominanceRatio (peak vs neighbor bins), spectralCentroid (power-weighted Hz),
// imaging14825Rms (14825 Hz imaging whistle — see REQUIREMENTS DIAG-03 / SRC-06).

#include "HfDiagnosticsHelpers.h"
#include <SchroederTank32.h>

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
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
using sendbloom::test::printThreePathCsvHeader;
using sendbloom::test::printThreePathCsvRow;
using sendbloom::test::renderTankPath;

constexpr float kImagingBandPeakRmsMax = 0.0022f;

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
