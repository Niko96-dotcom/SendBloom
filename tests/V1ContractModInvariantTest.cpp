#include "SchroederTank32DelayTable.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

TEST_CASE ("LFO depth seconds invariant at five processing rates",
           "[v1][contract][mod-invariant][DSP-05]")
{
    using Table = sendbloom::SchroederTank32DelayTable;

    const std::array<double, 5> rates { 32768.0, 44100.0, 48000.0, 88200.0, 96000.0 };

    for (const auto rate : rates)
    {
        const auto depthSamples = Table::tankLfoDepthSamplesForRate (rate);
        const auto depthSeconds = static_cast<double> (depthSamples) / rate;

        REQUIRE (depthSeconds == Catch::Approx (Table::kTankLfoDepthSeconds).margin (1.0e-6));

        if (rate == Table::kInternalRate)
            REQUIRE (depthSamples == Catch::Approx (16.0f).margin (1.0e-3f));
    }
}
