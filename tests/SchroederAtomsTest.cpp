#include <DampedComb.h>
#include <SchroederAllpass.h>
#include <SchroederTank32DelayTable.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <numeric>

namespace
{

constexpr auto kSampleRate = 48000.0;

bool areCoprimeEnough (int a, int b)
{
    auto x = a;
    auto y = b;

    while (y != 0)
    {
        const auto t = y;
        y = x % y;
        x = t;
    }

    return x == 1;
}

} // namespace

TEST_CASE ("SchroederTank32DelayTable has four APF and four comb lengths under 1 s", "[verb][atoms][delayTable]")
{
    for (const auto d : sendbloom::SchroederTank32DelayTable::kSeriesApfDelays)
    {
        REQUIRE (d > 0);
        REQUIRE (d < 32768);
    }

    for (const auto d : sendbloom::SchroederTank32DelayTable::kParallelCombDelays)
    {
        REQUIRE (d > 0);
        REQUIRE (d < 32768);
    }

    REQUIRE (sendbloom::SchroederTank32DelayTable::kTankApDelay > 0);
    REQUIRE (sendbloom::SchroederTank32DelayTable::kTankApDelay < 32768);

    const auto comb0 = sendbloom::SchroederTank32DelayTable::kParallelCombDelays[0];
    const auto comb1 = sendbloom::SchroederTank32DelayTable::kParallelCombDelays[1];
    REQUIRE (areCoprimeEnough (comb0, comb1));
}

TEST_CASE ("SchroederAllpass passes energy through diffusion", "[verb][atoms][allpass]")
{
    sendbloom::SchroederAllpass apf;
    apf.prepare (kSampleRate, 2048);
    apf.setDelay (200.0f);
    apf.setFeedback (0.7f);

    float energy = 0.0f;

    for (int i = 0; i < 512; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        const auto out = apf.processSample (in);
        energy += out * out;
    }

    REQUIRE (energy > 0.01f);
}

TEST_CASE ("DampedComb sustains decaying tail from impulse", "[verb][atoms][comb]")
{
    sendbloom::DampedComb comb;
    comb.prepare (kSampleRate, 8192);
    comb.setDelay (1200.0f);
    comb.setDampingCutoff (8000.0f);
    comb.setFeedbackForRT60 (1.5f, 1200.0f);

    float peak = 0.0f;

    for (int i = 0; i < 4800; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        peak = std::max (peak, std::abs (comb.processSample (in)));
    }

    REQUIRE (peak > 0.05f);
}
