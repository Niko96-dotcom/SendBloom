#pragma once

#include "Authentic32Mode.h"
#include "LegacyAccumulatorPath.h"
#include "RateConverterPair.h"
#include "SchroederTank32DelayTable.h"
#include "SchroederTankCore.h"
#include <algorithm>
#include <juce_core/juce_core.h>
#include <vector>

namespace sendbloom
{

class FixedRateAdapter
{
public:
    void prepare (double hostRate, int maxBlockSize) noexcept
    {
        hostRate_ = hostRate;
        maxBlockSize_ = maxBlockSize;

        core.prepare (SchroederTank32DelayTable::kInternalRate, maxBlockSize);
        converters.prepare (hostRate, maxBlockSize);
        legacy_.prepare (hostRate, maxBlockSize);
        hostScratch.assign (static_cast<size_t> (maxBlockSize), 0.0f);

        const auto maxInternal = converters.getMaxUpsampledLen (maxBlockSize);
        internalScratch.assign (static_cast<size_t> (maxInternal), 0.0);
        internalProcessBuf.assign (static_cast<size_t> (maxInternal), 0.0);
    }

    void processBlock (const float* in,
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
            {
                const int nInternal = converters.upsample (in, n, internalScratch.data());

                for (int i = 0; i < nInternal; ++i)
                {
                    internalProcessBuf[static_cast<size_t> (i)] = static_cast<double> (
                        core.processSample (static_cast<float> (internalScratch[static_cast<size_t> (i)]),
                                            rt60,
                                            darkMix));
                }

                std::fill (out, out + n, 0.0f);
                const int written = converters.downsample (internalProcessBuf.data(), nInternal, out, n);
                jassert (written >= 0 && written <= n);
                break;
            }
        }
    }

    void reset() noexcept
    {
        converters.reset();
        core.reset();
        legacy_.reset();
    }

    int getRoundTripLatencySamples() const noexcept
    {
        return converters.getRoundTripLatencySamples();
    }

private:
    SchroederTankCore core;
    RateConverterPair converters;
    LegacyAccumulatorPath legacy_;
    std::vector<float> hostScratch;
    std::vector<double> internalScratch;
    std::vector<double> internalProcessBuf;
    double hostRate_ { 48000.0 };
    int maxBlockSize_ { 512 };
};

} // namespace sendbloom
