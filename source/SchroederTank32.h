#pragma once

#include "Authentic32Mode.h"
#include "EngineCrossfade.h"
#include "FixedRateAdapter.h"
#include "HostRateReverbEngine.h"
#include "IReverbEngine.h"
#include <cmath>
#include <optional>
#include <vector>

namespace sendbloom
{

class SchroederTank32 : public IReverbEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept override
    {
        hostRate = sampleRate;
        maxBlockSize_ = maxBlockSize;

        hostEngine.prepare (sampleRate, maxBlockSize);
        fixedRate_.prepare (sampleRate, maxBlockSize);
        engineCrossfade_.prepare (sampleRate);
        hostCrossfadeScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        fixedCrossfadeScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    float processSample (float input,
                         float rt60Seconds,
                         float darkMix,
                         bool authenticColor) noexcept override
    {
        if (engineCrossfade_.isCrossfading())
        {
            float out = 0.0f;
            processBlock (&input, &out, 1, rt60Seconds, darkMix, authenticColor);
            return out;
        }

        if (! authenticColor)
            return hostEngine.processSample (input, rt60Seconds, darkMix, false);

        const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
        float out = 0.0f;
        fixedRate_.processBlock (&input, &out, 1, rt60Seconds, darkMix, mode);
        return out;
    }

    void processBlock (const float* input,
                       float* output,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor) noexcept override
    {
        if (numSamples > maxBlockSize_)
            return;

        if (engineCrossfade_.isCrossfading())
        {
            const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
            hostEngine.processBlock (input,
                                     hostCrossfadeScratch_.data(),
                                     numSamples,
                                     rt60Seconds,
                                     darkMix,
                                     false);
            fixedRate_.processBlock (input,
                                     fixedCrossfadeScratch_.data(),
                                     numSamples,
                                     rt60Seconds,
                                     darkMix,
                                     mode);
            engineCrossfade_.mixWetBlock (hostCrossfadeScratch_.data(),
                                          fixedCrossfadeScratch_.data(),
                                          output,
                                          numSamples);

            if (! engineCrossfade_.isCrossfading())
            {
                if (engineCrossfade_.targetIsFixedEngine())
                    hostEngine.reset();
                else
                    fixedRate_.reset();
            }

            return;
        }

        if (! authenticColor)
        {
            IReverbEngine::processBlock (input, output, numSamples, rt60Seconds, darkMix, authenticColor);
            return;
        }

        const auto mode = diagnosticsMode_.value_or (Authentic32Mode::ProperSRC);
        fixedRate_.processBlock (input, output, numSamples, rt60Seconds, darkMix, mode);
    }

    bool isCrossfading() const noexcept override
    {
        return engineCrossfade_.isCrossfading();
    }

    void requestEngineCrossfade (bool targetAuthentic) noexcept override
    {
        if (engineCrossfade_.isCrossfading()
            && engineCrossfade_.targetIsFixedEngine() == targetAuthentic)
            return;

        if (targetAuthentic)
            engineCrossfade_.beginCrossfadeTowardFixed();
        else
            engineCrossfade_.beginCrossfadeTowardHost();
    }

    void setAuthentic32ModeForDiagnostics (Authentic32Mode mode) noexcept
    {
        diagnosticsMode_ = mode;
    }

    void clearAuthentic32ModeForDiagnostics() noexcept
    {
        diagnosticsMode_ = std::nullopt;
    }

    int getSrcRoundTripLatencySamples() const noexcept
    {
        return fixedRate_.getRoundTripLatencySamples();
    }

private:
    HostRateReverbEngine hostEngine;
    FixedRateAdapter fixedRate_;
    EngineCrossfade engineCrossfade_;
    std::vector<float> hostCrossfadeScratch_;
    std::vector<float> fixedCrossfadeScratch_;
    std::optional<Authentic32Mode> diagnosticsMode_;
    double hostRate { 48000.0 };
    int maxBlockSize_ { 512 };
};

} // namespace sendbloom
