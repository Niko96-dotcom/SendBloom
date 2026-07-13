#pragma once

#include "RateConverterPair.h"
#include "SchroederTank32DelayTable.h"
#include "SchroederTankCore.h"
#include <algorithm>
#include <juce_core/juce_core.h>
#include <vector>

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
#include "Authentic32Mode.h"
#include "LegacyAccumulatorPath.h"
#endif

namespace sendbloom
{

class FixedRateAdapter
{
public:
    void prepare (double hostRate, int maxBlockSize) noexcept
    {
        maxBlockSize_ = maxBlockSize;

        core.prepare (SchroederTank32DelayTable::kInternalRate, maxBlockSize);
        converters.prepare (hostRate, maxBlockSize);
#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
        legacy_.prepare (hostRate, maxBlockSize);
#endif

        const auto maxInternal = converters.getMaxUpsampledLen (maxBlockSize);
        internalScratch.assign (static_cast<size_t> (maxInternal), 0.0);
        internalProcessBuf.assign (static_cast<size_t> (maxInternal), 0.0);
    }

    void processBlock (const float* in,
                       float* out,
                       int n,
                       float rt60,
                       float darkMix) noexcept
    {
        if (n > maxBlockSize_)
            return;

        processProperSrc (in, out, n, rt60, darkMix);
    }

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    void processBlockForDiagnostics (const float* in,
                                     float* out,
                                     int n,
                                     float rt60,
                                     float darkMix,
                                     Authentic32Mode mode) noexcept
    {
        if (n > maxBlockSize_)
            return;

        switch (mode)
        {
            case Authentic32Mode::Off:
                std::fill (out, out + n, 0.0f);
                break;

            case Authentic32Mode::LegacyAccumulator:
                legacy_.processBlock (in, out, n, rt60, darkMix);
                break;

            case Authentic32Mode::ProperSRC:
                processProperSrc (in, out, n, rt60, darkMix);
                break;
        }
    }
#endif

    void reset() noexcept
    {
        converters.reset();
        core.reset();
#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
        legacy_.reset();
#endif
    }

    int getRoundTripLatencySamples() const noexcept
    {
        return converters.getRoundTripLatencySamples();
    }

private:
    void processProperSrc (const float* in,
                           float* out,
                           int n,
                           float rt60,
                           float darkMix) noexcept
    {
        const int nInternal = converters.upsample (in, n, internalScratch.data());
        core.setParameters (rt60, darkMix);

        for (int i = 0; i < nInternal; ++i)
        {
            internalProcessBuf[static_cast<size_t> (i)] = static_cast<double> (
                core.processSample (static_cast<float> (internalScratch[static_cast<size_t> (i)])));
        }

        std::fill (out, out + n, 0.0f);
        const int written = converters.downsample (internalProcessBuf.data(), nInternal, out, n);
        jassert (written >= 0 && written <= n);
        juce::ignoreUnused (written);
    }

    SchroederTankCore core;
    RateConverterPair converters;
#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    LegacyAccumulatorPath legacy_;
#endif
    std::vector<double> internalScratch;
    std::vector<double> internalProcessBuf;
    int maxBlockSize_ { 512 };
};

} // namespace sendbloom
