#pragma once

#include <SchroederTankCore.h>
#include <cmath>
#include <vector>

namespace sendbloom::test::reverb
{

inline float measureRT60 (const std::vector<float>& samples, double sampleRate)
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

inline float maxAbsDiff (const std::vector<float>& a, const std::vector<float>& b)
{
    const auto n = std::min (a.size(), b.size());
    auto maxDiff = 0.0f;

    for (size_t i = 0; i < n; ++i)
        maxDiff = std::max (maxDiff, std::abs (a[i] - b[i]));

    return maxDiff;
}

inline std::vector<float> renderCoreImpulse (sendbloom::SchroederTankCore& core,
                                             float rt60,
                                             float darkMix,
                                             int numSamples)
{
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        out[static_cast<size_t> (i)] = core.processSample (in, rt60, darkMix);
    }

    return out;
}

} // namespace sendbloom::test::reverb
