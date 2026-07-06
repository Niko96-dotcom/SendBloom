#include "ChainTestHelpers.h"
#include <SchroederTank32.h>
#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;

float measureRT60 (const std::vector<float>& samples, double sampleRate)
{
    if (samples.size() < 64)
        return 0.0f;

    std::vector<double> schroeder (samples.size());
    double tailEnergy = 0.0;

    for (int i = static_cast<int> (samples.size()) - 1; i >= 0; --i)
    {
        tailEnergy += static_cast<double> (samples[static_cast<size_t> (i)])
                      * static_cast<double> (samples[static_cast<size_t> (i)]);
        schroeder[static_cast<size_t> (i)] = tailEnergy;
    }

    const auto start = static_cast<size_t> (sampleRate * 0.02);
    const auto end = schroeder.size() * 3 / 4;

    if (start >= end || schroeder[start] <= 0.0)
        return 0.0f;

    const auto targetDb = -60.0;
    const auto eStart = schroeder[start];
    const auto eEnd = schroeder[std::max (start + 1, end - 1)];

    if (eEnd <= 0.0 || eStart <= eEnd)
        return 0.0f;

    const auto dbStart = 10.0 * std::log10 (eStart);
    const auto dbEnd = 10.0 * std::log10 (eEnd);
    const auto slope = (dbEnd - dbStart) / static_cast<double> (end - start);

    if (slope >= -1.0e-9)
        return 0.0f;

    const auto dbAtStart = 10.0 * std::log10 (eStart);
    const auto samplesTo60 = (targetDb - dbAtStart) / slope;

    return static_cast<float> (samplesTo60 / sampleRate);
}

std::vector<float> renderImpulseResponse (sendbloom::SchroederTank32& tank,
                                          float rt60,
                                          float darkMix,
                                          bool authentic,
                                          int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = tank.processSample (in, rt60, darkMix, authentic);
    }

    return out;
}

} // namespace

TEST_CASE ("SchroederTank32 impulse produces audible tail", "[verb][SchroederTank32]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto ir = renderImpulseResponse (tank, rt60, 0.0f, false, 48000);

    REQUIRE (sendbloom::test::rms (ir) > 1.0e-4f);
}

TEST_CASE ("SchroederTank32 Dark mode adds predelay energy vs Bright", "[verb][SchroederTank32][dark]")
{
    sendbloom::SchroederTank32 brightTank;
    sendbloom::SchroederTank32 darkTank;
    brightTank.prepare (kSampleRate, 512);
    darkTank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto brightIr = renderImpulseResponse (brightTank, rt60, 0.0f, false, 8192);
    const auto darkIr = renderImpulseResponse (darkTank, rt60, 1.0f, false, 8192);

    const auto predelaySamples = static_cast<int> (0.055 * kSampleRate);
  const auto earlyBright = sendbloom::test::rms (std::vector<float> (brightIr.begin(), brightIr.begin() + static_cast<size_t> (predelaySamples / 4)));
    const auto earlyDark = sendbloom::test::rms (std::vector<float> (darkIr.begin() + static_cast<size_t> (predelaySamples / 2), darkIr.begin() + static_cast<size_t> (predelaySamples)));

    REQUIRE (earlyDark < earlyBright + 0.05f);
    REQUIRE (sendbloom::test::rms (darkIr) > 1.0e-4f);
}

TEST_CASE ("SchroederTank32 authentic_color path stays finite", "[verb][SchroederTank32][authentic]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    for (int i = 0; i < 24000; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        const auto out = tank.processSample (in, rt60, 0.0f, true);
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
    const auto ir = renderImpulseResponse (tank, target, 0.0f, false, static_cast<int> (kSampleRate * (target * 3.0f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTank32 RT60 within tolerance at size 0.5", "[verb][SchroederTank32][rt60]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (tank, target, 0.0f, false, static_cast<int> (kSampleRate * (target * 3.0f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}

TEST_CASE ("SchroederTank32 RT60 within tolerance at size 1.0", "[verb][SchroederTank32][rt60]")
{
    sendbloom::SchroederTank32 tank;
    tank.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 1.0f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (tank, target, 0.0f, false, static_cast<int> (kSampleRate * (target * 2.5f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.15f));
}
