#pragma once

#include "EnvelopeDetector.h"
#include "Fdn8Reverb.h"
#include "IReverbEngine.h"
#include "NoiseGate.h"
#include "PressureSend.h"
#include "SchroederTank32.h"
#include "WetOverdrive.h"
#include <memory>
#include <vector>

namespace sendbloom
{

class GatedBloomChain
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        if (reverb == nullptr)
            reverb = std::make_unique<SchroederTank32>();

        reverb->prepare (sampleRate, maxBlockSize);
        envelope.prepare (sampleRate, 5.0f, 10.0f);
        preGate.prepare (sampleRate, GateProfile::PreSoft);
        postGate.prepare (sampleRate, GateProfile::PostHard);
        overdrive.prepare (sampleRate);

        maxBlockSize_ = maxBlockSize;
        wetSendScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        reverbScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    }

    void setReverbEngineForTests (std::unique_ptr<IReverbEngine> engine) noexcept
    {
        reverb = std::move (engine);
    }

    void requestEngineCrossfade (bool targetAuthentic) noexcept
    {
        reverb->requestEngineCrossfade (targetAuthentic);
    }

    bool isCrossfading() const noexcept
    {
        return reverb != nullptr && reverb->isCrossfading();
    }

    int getSrcRoundTripLatencySamples() const noexcept
    {
        return reverb != nullptr ? reverb->getSrcRoundTripLatencySamples() : 0;
    }

    EnvelopeDetector& getEnvelope() noexcept { return envelope; }
    const EnvelopeDetector& getEnvelope() const noexcept { return envelope; }

    float processSample (float input,
                         float inputEnvelope,
                         float rt60Seconds,
                         float darkModeMix,
                         bool authenticColor,
                         float distnBlend,
                         float sendGain,
                         bool gatePreSoft,
                         float thresholdDb) noexcept
    {
        auto wet = input;

        if (gatePreSoft)
            wet *= preGate.process (inputEnvelope, thresholdDb);

        wet = PressureSend::process (wet, sendGain);
        wet = reverb->processSample (wet, rt60Seconds, darkModeMix, authenticColor);
        wet = overdrive.process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= postGate.process (inputEnvelope, thresholdDb);

        return wet;
    }

    void processBlock (const float* monoIn,
                       const float* envelope,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor,
                       float distnBlend,
                       float sendGain,
                       bool gatePreSoft,
                       float thresholdDb) noexcept
    {
        processBlock (monoIn, envelope, wetOut, numSamples, rt60Seconds, darkMix, authenticColor,
                      distnBlend, static_cast<const float*> (nullptr), sendGain, gatePreSoft, thresholdDb);
    }

    void processBlock (const float* monoIn,
                       const float* envelope,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor,
                       float distnBlend,
                       const float* sendGains,
                       bool gatePreSoft,
                       float thresholdDb) noexcept
    {
        processBlock (monoIn, envelope, wetOut, numSamples, rt60Seconds, darkMix, authenticColor,
                      distnBlend, sendGains, 1.0f, gatePreSoft, thresholdDb);
    }

private:
    void processBlock (const float* monoIn,
                       const float* envelope,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor,
                       float distnBlend,
                       const float* sendGains,
                       float constantSendGain,
                       bool gatePreSoft,
                       float thresholdDb) noexcept
    {
        if (numSamples > maxBlockSize_)
            return;

        const auto sampleSendGain = [sendGains, constantSendGain] (int i) noexcept
        {
            return sendGains != nullptr ? sendGains[static_cast<size_t> (i)] : constantSendGain;
        };

        if (! authenticColor && ! reverb->isCrossfading())
        {
            for (int i = 0; i < numSamples; ++i)
                wetOut[i] = processSample (monoIn[i], envelope[i], rt60Seconds, darkMix, false,
                                           distnBlend, sampleSendGain (i), gatePreSoft, thresholdDb);
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            auto wet = monoIn[i];

            if (gatePreSoft)
                wet *= preGate.process (envelope[i], thresholdDb);

            wetSendScratch_[static_cast<size_t> (i)] = PressureSend::process (wet, sampleSendGain (i));
        }

        reverb->processBlock (wetSendScratch_.data(), reverbScratch_.data(), numSamples,
                              rt60Seconds, darkMix, true);

        for (int i = 0; i < numSamples; ++i)
        {
            auto wet = overdrive.process (reverbScratch_[static_cast<size_t> (i)], distnBlend);

            if (! gatePreSoft)
                wet *= postGate.process (envelope[i], thresholdDb);

            wetOut[i] = wet;
        }
    }

    std::unique_ptr<IReverbEngine> reverb;
    EnvelopeDetector envelope;
    NoiseGate preGate;
    NoiseGate postGate;
    WetOverdriveState overdrive;
    int maxBlockSize_ = 0;
    std::vector<float> wetSendScratch_;
    std::vector<float> reverbScratch_;
};

} // namespace sendbloom
