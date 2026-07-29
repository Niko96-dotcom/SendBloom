#include "Fv1RingTank.h"
#include "Fv1RingTankTable.h"
#include "ParameterCurves.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <vector>

namespace
{

constexpr double kRate = sendbloom::Fv1RingTankTable::kInternalRate;

float measureOnsetSeconds (const std::vector<float>& samples,
                           double sampleRate,
                           float threshold = 1.0e-4f)
{
    for (size_t i = 0; i < samples.size(); ++i)
    {
        if (std::abs (samples[i]) > threshold)
            return static_cast<float> (static_cast<double> (i) / sampleRate);
    }

    return static_cast<float> (samples.size()) / static_cast<float> (sampleRate);
}

std::vector<float> renderTankImpulse (sendbloom::Fv1RingTank& tank,
                                      float rt60,
                                      float darkMix,
                                      int numSamples)
{
    tank.setParameters (rt60, darkMix);
    std::vector<float> ir (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
        ir[static_cast<size_t> (i)] = tank.processSample (i == 0 ? 1.0f : 0.0f);

    return ir;
}

float crestFactor (const std::vector<float>& samples, size_t start, size_t end)
{
    if (start >= end || end > samples.size())
        return 0.0f;

    auto peak = 0.0;
    auto sumSq = 0.0;
    const auto n = static_cast<double> (end - start);

    for (size_t i = start; i < end; ++i)
    {
        const auto x = static_cast<double> (samples[i]);
        peak = std::max (peak, std::abs (x));
        sumSq += x * x;
    }

    const auto rms = std::sqrt (sumSq / n);
    return rms > 0.0 ? static_cast<float> (peak / rms) : 0.0f;
}

float kurtosis (const std::vector<float>& samples, size_t start, size_t end)
{
    if (start >= end || end > samples.size())
        return 0.0f;

    const auto n = static_cast<double> (end - start);
    auto mean = 0.0;

    for (size_t i = start; i < end; ++i)
        mean += static_cast<double> (samples[i]);

    mean /= n;

    auto m2 = 0.0;
    auto m4 = 0.0;

    for (size_t i = start; i < end; ++i)
    {
        const auto d = static_cast<double> (samples[i]) - mean;
        const auto d2 = d * d;
        m2 += d2;
        m4 += d2 * d2;
    }

    m2 /= n;
    m4 /= n;

    return m2 > 0.0 ? static_cast<float> (m4 / (m2 * m2)) : 0.0f;
}

} // namespace

TEST_CASE ("Fv1RingTank two-allpass ring raises early density within contracts",
           "[verb][Fv1RingTank][fv1][density]")
{
    STATIC_REQUIRE (sendbloom::Fv1RingTankTable::loopSamples() == 27086);
    STATIC_REQUIRE (sendbloom::Fv1RingTankTable::totalRamWords() == 31532);
    STATIC_REQUIRE (sendbloom::Fv1RingTankTable::sumOf (
                        sendbloom::Fv1RingTankTable::kRingAllpassDelays)
                    == 10804);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    constexpr int numSamples = static_cast<int> (kRate); // 1 s

    sendbloom::Fv1RingTank bright;
    sendbloom::Fv1RingTank dark;
    bright.prepare (kRate, 512);
    dark.prepare (kRate, 512);

    const auto brightIr = renderTankImpulse (bright, rt60, 0.0f, numSamples);
    const auto darkIr = renderTankImpulse (dark, rt60, 1.0f, numSamples);

    const auto allFinite = [] (const std::vector<float>& samples)
    {
        return std::all_of (samples.begin(), samples.end(), [] (float sample)
        {
            return std::isfinite (sample);
        });
    };
    const auto peak = [] (const std::vector<float>& samples)
    {
        auto result = 0.0f;

        for (const auto sample : samples)
            result = std::max (result, std::abs (sample));

        return result;
    };

    REQUIRE (allFinite (brightIr));
    REQUIRE (allFinite (darkIr));
    REQUIRE (peak (brightIr) < 8.0f);
    REQUIRE (peak (darkIr) < 8.0f);

    const auto brightOnset = measureOnsetSeconds (brightIr, kRate);
    const auto darkOnset = measureOnsetSeconds (darkIr, kRate);
    const auto darkMinusBright = darkOnset - brightOnset;

    INFO ("bright onset ms = " << brightOnset * 1000.0f);
    INFO ("dark-bright onset ms = " << darkMinusBright * 1000.0f);
    REQUIRE (brightOnset >= 0.005f);
    REQUIRE (brightOnset <= 0.010f);
    REQUIRE (darkMinusBright >= 0.050f);
    REQUIRE (darkMinusBright <= 0.060f);

    const auto winStart = static_cast<size_t> (std::lround (0.100 * kRate));
    const auto winEnd = static_cast<size_t> (std::lround (0.250 * kRate));
    const auto brightCrest = crestFactor (brightIr, winStart, winEnd);
    const auto brightKurt = kurtosis (brightIr, winStart, winEnd);
    const auto darkCrest = crestFactor (darkIr, winStart, winEnd);
    const auto darkKurt = kurtosis (darkIr, winStart, winEnd);

    INFO ("bright crest factor (100-250 ms) = " << brightCrest);
    INFO ("bright kurtosis (100-250 ms) = " << brightKurt);
    INFO ("dark crest factor (100-250 ms) = " << darkCrest);
    INFO ("dark kurtosis (100-250 ms) = " << darkKurt);
    REQUIRE (brightCrest < 7.0f);
    REQUIRE (brightKurt < 10.5f);
    REQUIRE (darkCrest < 7.0f);
    REQUIRE (darkKurt < 12.0f);
}
