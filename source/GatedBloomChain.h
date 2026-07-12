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
                       const float* envelopeIn,
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
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix, authenticColor,
                      nullptr, distnBlend, nullptr, sendGain, nullptr, thresholdDb, gatePreSoft);
    }

    void processBlock (const float* monoIn,
                       const float* envelopeIn,
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
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix, authenticColor,
                      nullptr, distnBlend, sendGains, 1.0f, nullptr, thresholdDb, gatePreSoft);
    }

    /** ADR-V1-06: per-sample distn / send / threshold arrays (RT-06). */
    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor,
                       const float* distnBlends,
                       const float* sendGains,
                       const float* thresholdDbs,
                       bool gatePreSoft) noexcept
    {
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix, authenticColor,
                      distnBlends, 0.0f, sendGains, 1.0f, thresholdDbs, 0.0f, gatePreSoft);
    }

private:
    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       bool authenticColor,
                       const float* distnBlends,
                       float constantDistn,
                       const float* sendGains,
                       float constantSendGain,
                       const float* thresholdDbs,
                       float constantThresholdDb,
                       bool gatePreSoft) noexcept
    {
        if (numSamples > maxBlockSize_)
            return;

        const auto sampleDistn = [distnBlends, constantDistn] (int i) noexcept
        {
            return distnBlends != nullptr ? distnBlends[static_cast<size_t> (i)] : constantDistn;
        };
        const auto sampleSendGain = [sendGains, constantSendGain] (int i) noexcept
        {
            return sendGains != nullptr ? sendGains[static_cast<size_t> (i)] : constantSendGain;
        };
        const auto sampleThreshold = [thresholdDbs, constantThresholdDb] (int i) noexcept
        {
            return thresholdDbs != nullptr ? thresholdDbs[static_cast<size_t> (i)]
                                          : constantThresholdDb;
        };

        if (! authenticColor && ! reverb->isCrossfading())
        {
            for (int i = 0; i < numSamples; ++i)
                wetOut[i] = processSample (monoIn[i], envelopeIn[i], rt60Seconds, darkMix, false,
                                           sampleDistn (i), sampleSendGain (i), gatePreSoft,
                                           sampleThreshold (i));
            return;
        }

        for (int i = 0; i < numSamples; ++i)
        {
            auto wet = monoIn[i];

            if (gatePreSoft)
                wet *= preGate.process (envelopeIn[i], sampleThreshold (i));

            wetSendScratch_[static_cast<size_t> (i)] = PressureSend::process (wet, sampleSendGain (i));
        }

        reverb->processBlock (wetSendScratch_.data(), reverbScratch_.data(), numSamples,
                              rt60Seconds, darkMix, true);

        for (int i = 0; i < numSamples; ++i)
        {
            auto wet = overdrive.process (reverbScratch_[static_cast<size_t> (i)], sampleDistn (i));

            if (! gatePreSoft)
                wet *= postGate.process (envelopeIn[i], sampleThreshold (i));

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
