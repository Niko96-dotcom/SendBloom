#include "ChainTestHelpers.h"
#include <Fdn8Reverb.h>
#include <IReverbEngine.h>
#include <ParameterCurves.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <memory>
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

    const auto samplesTo60 = (targetDb - dbStart) / slope;
    return static_cast<float> (samplesTo60 / sampleRate);
}

std::vector<float> renderImpulseResponse (sendbloom::IReverbEngine& engine,
                                          float rt60,
                                          float darkMix,
                                          int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = engine.processSample (in, rt60, darkMix, false);
    }

    return out;
}

} // namespace

TEST_CASE ("Fdn8Reverb impulse produces audible tail", "[verb][Fdn8]")
{
    sendbloom::Fdn8Reverb fdn;
    fdn.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto ir = renderImpulseResponse (fdn, rt60, 0.0f, 48000);

    REQUIRE (sendbloom::test::rms (ir) > 1.0e-4f);
}

TEST_CASE ("Fdn8Reverb RT60 within tolerance at size 0.5", "[verb][Fdn8][rt60]")
{
    sendbloom::Fdn8Reverb fdn;
    fdn.prepare (kSampleRate, 512);

    constexpr float sizeNorm = 0.5f;
    const auto target = sendbloom::ParameterCurves::sizeToRT60 (sizeNorm);
    const auto ir = renderImpulseResponse (fdn, target, 0.0f, static_cast<int> (kSampleRate * (target * 3.0f)));
    const auto measured = measureRT60 (ir, kSampleRate);

    REQUIRE (measured == Catch::Approx (target).margin (target * 0.20f));
}

TEST_CASE ("Fdn8Reverb implements IReverbEngine polymorphically", "[verb][Fdn8][interface]")
{
    std::unique_ptr<sendbloom::IReverbEngine> engine = std::make_unique<sendbloom::Fdn8Reverb>();
    engine->prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    const auto ir = renderImpulseResponse (*engine, rt60, 0.0f, 4096);

    REQUIRE (sendbloom::test::rms (ir) > 1.0e-4f);
}

TEST_CASE ("Fdn8Reverb dark mode stays finite", "[verb][Fdn8][dark]")
{
    sendbloom::Fdn8Reverb fdn;
    fdn.prepare (kSampleRate, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    for (int i = 0; i < 24000; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        const auto out = fdn.processSample (in, rt60, 1.0f, false);
        REQUIRE (std::isfinite (out));
        REQUIRE (std::abs (out) < 8.0f);
    }
}
