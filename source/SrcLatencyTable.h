#pragma once

#include <array>
#include <cstddef>

namespace sendbloom
{

// Measured round-trip SRC priming delay per supported host rate.
// Values are the sum of upsampler and downsampler getInLenBeforeOutPos(0)
// after prepare(hostRate, kMaxHostBlock) — not CDSPResampler::getLatency()
// (which always returns 0). Offline r8brain probe 2026-07-08.

struct LatencyRow
{
    double hostRateHz;
    int roundTripSamples;
};

inline constexpr int kMaxHostBlock = 512;

inline constexpr std::array<LatencyRow, 4> kMeasuredLatencyTable { {
    { 44100.0, 5208 },
    { 48000.0, 5160 },
    { 88200.0, 8915 },
    { 96000.0, 8670 },
} };

inline constexpr int lookupRoundTripLatencySamples (double hostRateHz) noexcept
{
    for (const auto& row : kMeasuredLatencyTable)
    {
        if (row.hostRateHz == hostRateHz)
            return row.roundTripSamples;
    }

    return 0;
}

} // namespace sendbloom
