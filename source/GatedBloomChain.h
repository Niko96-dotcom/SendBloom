#pragma once

#include "EnvelopeDetector.h"
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
        // Fast detector release so the post "hard close" isn't preceded by a long
        // envelope decay before the gate decides to shut. The gate's hold stage
        // (NoiseGate::kHoldMs) bridges the inter-peak dips of low notes so this
        // fast release doesn't stutter a sustained chord.
        envelope.prepare (sampleRate, 1.0f, 2.0f);
        gate.prepare (sampleRate, GateProfile::PostHard);
        overdrive.prepare (sampleRate);

        maxBlockSize_ = maxBlockSize;
        wetSendScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        reverbScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        gateGainScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    }

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    void setReverbEngineForTests (std::unique_ptr<IReverbEngine> engine) noexcept
    {
        reverb = std::move (engine);
    }
#endif

    EnvelopeDetector& getEnvelope() noexcept { return envelope; }
    const EnvelopeDetector& getEnvelope() const noexcept { return envelope; }

    float processSample (float input,
                         float inputEnvelope,
                         float rt60Seconds,
                         float darkModeMix,
                         float distnBlend,
                         float sendGain,
                         bool gatePreSoft,
                         float thresholdDb) noexcept
    {
        // One shared gate whose profile follows the Gate switch; its running
        // state carries across mode toggles (ADR-V1-11: single movable circuit).
        gate.setProfile (gatePreSoft ? GateProfile::PreSoft : GateProfile::PostHard);
        const auto gateGain = gate.process (inputEnvelope, thresholdDb);

        auto wet = input;

        if (gatePreSoft)
            wet *= gateGain;

        wet = PressureSend::process (wet, sendGain);
        wet = reverb->processSample (wet, rt60Seconds, darkModeMix);
        wet = overdrive.process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= gateGain;

        return wet;
    }

    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       float distnBlend,
                       float sendGain,
                       bool gatePreSoft,
                       float thresholdDb) noexcept
    {
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      nullptr, distnBlend, nullptr, sendGain, nullptr, thresholdDb, gatePreSoft);
    }

    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       float distnBlend,
                       const float* sendGains,
                       bool gatePreSoft,
                       float thresholdDb) noexcept
    {
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      nullptr, distnBlend, sendGains, 1.0f, nullptr, thresholdDb, gatePreSoft);
    }

    /** ADR-V1-06: per-sample distn / send / threshold arrays (RT-06). */
    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       const float* distnBlends,
                       const float* sendGains,
                       const float* thresholdDbs,
                       bool gatePreSoft) noexcept
    {
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      distnBlends, 0.0f, sendGains, 1.0f, thresholdDbs, 0.0f, gatePreSoft);
    }

private:
    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
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

        gate.setProfile (gatePreSoft ? GateProfile::PreSoft : GateProfile::PostHard);

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance the single gate once per sample here so its state stays
            // coherent; apply pre now, cache the gain for the post node below.
            const auto g = gate.process (envelopeIn[i], sampleThreshold (i));
            gateGainScratch_[static_cast<size_t> (i)] = g;

            auto wet = monoIn[i];

            if (gatePreSoft)
                wet *= g;

            wetSendScratch_[static_cast<size_t> (i)] = PressureSend::process (wet, sampleSendGain (i));
        }

        reverb->processBlock (wetSendScratch_.data(), reverbScratch_.data(), numSamples,
                              rt60Seconds, darkMix);

        for (int i = 0; i < numSamples; ++i)
        {
            auto wet = overdrive.process (reverbScratch_[static_cast<size_t> (i)], sampleDistn (i));

            if (! gatePreSoft)
                wet *= gateGainScratch_[static_cast<size_t> (i)];

            wetOut[i] = wet;
        }
    }

    std::unique_ptr<IReverbEngine> reverb;
    EnvelopeDetector envelope;
    NoiseGate gate;
    WetOverdriveState overdrive;
    int maxBlockSize_ = 0;
    std::vector<float> wetSendScratch_;
    std::vector<float> reverbScratch_;
    std::vector<float> gateGainScratch_;
};

} // namespace sendbloom
