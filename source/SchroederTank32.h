#pragma once

#include "FixedRateAdapter.h"
#include "IReverbEngine.h"
#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
#include "Authentic32Mode.h"
#include <optional>
#endif

namespace sendbloom
{

class SchroederTank32 : public IReverbEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept override
    {
        maxBlockSize_ = maxBlockSize;
        fixedRate_.prepare (sampleRate, maxBlockSize);
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix) noexcept override
    {
        float out = 0.0f;
        processBlock (&input, &out, 1, rt60Seconds, darkMix);
        return out;
    }

    void processBlock (const float* input,
                       float* output,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix) noexcept override
    {
        if (numSamples > maxBlockSize_)
            return;

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
        if (diagnosticsMode_.has_value())
        {
            fixedRate_.processBlockForDiagnostics (input, output, numSamples, rt60Seconds, darkMix,
                                                   *diagnosticsMode_);
            return;
        }
#endif

        fixedRate_.processBlock (input, output, numSamples, rt60Seconds, darkMix);
    }

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    void setAuthentic32ModeForDiagnostics (Authentic32Mode mode) noexcept
    {
        diagnosticsMode_ = mode;
    }

    void clearAuthentic32ModeForDiagnostics() noexcept
    {
        diagnosticsMode_ = std::nullopt;
    }
#endif

private:
    FixedRateAdapter fixedRate_;
#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    std::optional<Authentic32Mode> diagnosticsMode_;
#endif
    int maxBlockSize_ { 512 };
};

} // namespace sendbloom
