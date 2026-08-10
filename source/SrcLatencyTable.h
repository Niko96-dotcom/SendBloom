#pragma once

#include "RateConverterPair.h"
#include <array>
#include <cstddef>

namespace sendbloom
{

// Canonical regression samples for the measured host-domain round-trip SRC
// priming delay. Production PDC always queries the prepared RateConverterPair
// live; this table pins the four supported-rate measurements made with
// kProperSrcQuality (TB=25%, Atten=90 dB, linear-phase) and
// maxHostBlock=512. Upsampler priming is already in host samples; downsampler
// priming is scaled from 32,768 Hz → host. Offline probe 2026-07-09.

struct LatencyRow
{
    double hostRateHz;
    int roundTripSamples;
};

inline constexpr int kMaxHostBlock = 512;

inline constexpr std::array<LatencyRow, 4> kMeasuredLatencyTable { {
    { 44100.0, 181 },
    { 48000.0, 186 },
    { 88200.0, 363 },
    { 96000.0, 372 },
} };

inline constexpr int lookupRoundTripLatencySamples (double hostRateHz) noexcept
{
    for (const auto& row : kMeasuredLatencyTable)
    {
        if (hostRateHz >= row.hostRateHz - 0.5 && hostRateHz <= row.hostRateHz + 0.5)
            return row.roundTripSamples;
    }

    return 0;
}

} // namespace sendbloom
