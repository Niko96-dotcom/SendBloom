#include "ChainTestHelpers.h"
#include "ReverbTestHelpers.h"
#include <SchroederTank32.h>
#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;

using sendbloom::test::reverb::measureRT60;

std::vector<float> renderImpulseResponse (sendbloom::SchroederTank32& tank,
                                          float rt60,
                                          float darkMix,
                                          int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = tank.processSample (in, rt60, darkMix);
    }

    return out;
}

} // namespace

TEST_CASE ("SchroederTank32 impulse produces audible tail", "[verb][SchroederTank32]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto ir = renderImpulseResponse (tank, rt60, 0.0f, 48000);

    REQUIRE (sendbloom::test::rms (ir) > 1.0e-4f);
}

TEST_CASE ("SchroederTank32 Dark mode adds predelay energy vs Bright", "[verb][SchroederTank32][dark]")
{
    sendbloom::SchroederTank32 brightTank;
    sendbloom::SchroederTank32 darkTank;
    brightTank.prepare (kSampleRate, 512);
    darkTank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto brightIr = renderImpulseResponse (brightTank, rt60, 0.0f, 8192);
    const auto darkIr = renderImpulseResponse (darkTank, rt60, 1.0f, 8192);

    const auto predelaySamples = static_cast<int> (0.055 * kSampleRate);
  const auto earlyBright = sendbloom::test::rms (std::vector<float> (brightIr.begin(), brightIr.begin() + static_cast<size_t> (predelaySamples / 4)));
    const auto earlyDark = sendbloom::test::rms (std::vector<float> (darkIr.begin() + static_cast<size_t> (predelaySamples / 2), darkIr.begin() + static_cast<size_t> (predelaySamples)));

    REQUIRE (earlyDark < earlyBright + 0.05f);
    REQUIRE (sendbloom::test::rms (darkIr) > 1.0e-4f);
}

TEST_CASE ("SchroederTank32 fixed ProperSRC path stays finite", "[verb][SchroederTank32][proper-src]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    for (int i = 0; i < 24000; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        const auto out = tank.processSample (in, rt60, 0.0f);
        REQUIRE (std::isfinite (out));
        REQUIRE (std::abs (out) < 8.0f);
    }
}

TEST_CASE ("SchroederTank32 RT60 within tolerance at size 0.25", "[verb][SchroederTank32][rt60]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 0.25f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (tank, target, 0.0f, static_cast<int> (kSampleRate * (target * 3.0f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTank32 RT60 within tolerance at size 0.5", "[verb][SchroederTank32][rt60]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (tank, target, 0.0f, static_cast<int> (kSampleRate * (target * 3.0f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTank32 RT60 within tolerance at size 1.0", "[verb][SchroederTank32][rt60]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 1.0f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (tank, target, 0.0f, static_cast<int> (kSampleRate * (target * 2.5f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}
