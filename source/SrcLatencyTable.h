#pragma once

#include "RateConverterPair.h"
#include <array>
#include <cstddef>

namespace sendbloom
{

// Measured host-domain round-trip SRC priming delay per supported host rate.
// Values are RateConverterPair::getRoundTripLatencySamples() after
// prepare(hostRate, kMaxHostBlock) with kProperSrcQuality (TB=25%, Atten=90 dB,
// linear-phase). Upsampler priming is already in host samples; downsampler
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
        if (row.hostRateHz == hostRateHz)
            return row.roundTripSamples;
    }

    return 0;
}

// Product policy (ADR-003 Path B): with kProperSrcQuality the wet-only SRC delay
// is ~3.9–4.1 ms. RC1 keeps setLatencySamples(0) always (CHN-04). Diagnostic
// getRoundTripLatencySamples() remains available for LAT-01.
inline constexpr int kMaxAcceptableReportedSrcLatencySamples = 0;

} // namespace sendbloom
