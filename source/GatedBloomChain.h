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
        gate.prepare (sampleRate);
        overdrive.prepare (sampleRate);

        maxBlockSize_ = maxBlockSize;
        wetSendScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        reverbScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
        postGateScratch_.assign (static_cast<size_t> (maxBlockSize), 0.0f);
    }

#if defined(SENDBLOOM_ENABLE_DIAGNOSTICS) && SENDBLOOM_ENABLE_DIAGNOSTICS
    void setReverbEngineForTests (std::unique_ptr<IReverbEngine> engine) noexcept
    {
        reverb = std::move (engine);
    }
#endif

    EnvelopeDetector& getEnvelope() noexcept { return envelope; }
    const EnvelopeDetector& getEnvelope() const noexcept { return envelope; }

    int getPdcLatencySamples() const noexcept
    {
        return reverb != nullptr ? reverb->getPdcLatencySamples() : 0;
    }

    /** Placement crossfade (ADR-V1-11c).

        `postDepth` is 0 with the gate ahead of the effect and 1 with it behind,
        and the Gate switch ramps between them rather than jumping. One gate gain
        `g` drives both nodes:

            pre  = g + (1 - g) * postDepth      (g when postDepth == 0, else 1)
            post = 1 - (1 - g) * postDepth      (1 when postDepth == 0, else g)

        Both are continuous in `postDepth`, and both collapse to unity while the
        gate is open, so the ramp only does work during a close. Without it,
        flipping the switch over a live tail stepped the wet output by up to 0.44
        in one sample — the post node muting or unmuting a full-level tail
        instantly. */
    static constexpr float postDepthFor (bool gatePre) noexcept { return gatePre ? 0.0f : 1.0f; }

    float processSample (float input,
                         float inputEnvelope,
                         float rt60Seconds,
                         float darkModeMix,
                         float distnBlend,
                         float sendGain,
                         bool gatePre,
                         float thresholdDb) noexcept
    {
        const auto postDepth = postDepthFor (gatePre);
        const auto g = gate.process (inputEnvelope, thresholdDb);

        auto wet = input * (g + (1.0f - g) * postDepth);
        wet = PressureSend::process (wet, sendGain);
        wet = reverb->processSample (wet, rt60Seconds, darkModeMix);
        wet = overdrive.process (wet, distnBlend);

        return wet * (1.0f - (1.0f - g) * postDepth);
    }

    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       float distnBlend,
                       float sendGain,
                       bool gatePre,
                       float thresholdDb) noexcept
    {
        const auto thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      nullptr, distnBlend, nullptr, sendGain, nullptr, thresholdLinear,
                      nullptr, postDepthFor (gatePre));
    }

    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       float distnBlend,
                       const float* sendGains,
                       bool gatePre,
                       float thresholdDb) noexcept
    {
        const auto thresholdLinear = juce::Decibels::decibelsToGain (thresholdDb);
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      nullptr, distnBlend, sendGains, 1.0f, nullptr, thresholdLinear,
                      nullptr, postDepthFor (gatePre));
    }

    /** ADR-V1-06: per-sample distn / send / linear-threshold arrays (RT-06),
        plus the per-sample gate placement depth (ADR-V1-11c). */
    void processBlock (const float* monoIn,
                       const float* envelopeIn,
                       float* wetOut,
                       int numSamples,
                       float rt60Seconds,
                       float darkMix,
                       const float* distnBlends,
                       const float* sendGains,
                       const float* thresholdLinears,
                       const float* gatePostDepths) noexcept
    {
        processBlock (monoIn, envelopeIn, wetOut, numSamples, rt60Seconds, darkMix,
                      distnBlends, 0.0f, sendGains, 1.0f, thresholdLinears, 0.0f,
                      gatePostDepths, 0.0f);
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
                       const float* thresholdLinears,
                       float constantThresholdLinear,
                       const float* postDepths,
                       float constantPostDepth) noexcept
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
        const auto sampleThreshold = [thresholdLinears, constantThresholdLinear] (int i) noexcept
        {
            return thresholdLinears != nullptr ? thresholdLinears[static_cast<size_t> (i)]
                                               : constantThresholdLinear;
        };
        const auto samplePostDepth = [postDepths, constantPostDepth] (int i) noexcept
        {
            return postDepths != nullptr ? postDepths[static_cast<size_t> (i)] : constantPostDepth;
        };

        for (int i = 0; i < numSamples; ++i)
        {
            // Advance the single gate once per sample here so its state stays
            // coherent; apply the pre node now, cache the post node below.
            const auto g = gate.processLinear (envelopeIn[i], sampleThreshold (i));
            const auto postDepth = samplePostDepth (i);
            postGateScratch_[static_cast<size_t> (i)] = 1.0f - (1.0f - g) * postDepth;

            const auto wet = monoIn[i] * (g + (1.0f - g) * postDepth);
            wetSendScratch_[static_cast<size_t> (i)] = PressureSend::process (wet, sampleSendGain (i));
        }

        reverb->processBlock (wetSendScratch_.data(), reverbScratch_.data(), numSamples,
                              rt60Seconds, darkMix);

        for (int i = 0; i < numSamples; ++i)
        {
            const auto wet = overdrive.process (reverbScratch_[static_cast<size_t> (i)], sampleDistn (i));
            wetOut[i] = wet * postGateScratch_[static_cast<size_t> (i)];
        }
    }

    std::unique_ptr<IReverbEngine> reverb;
    EnvelopeDetector envelope;
    NoiseGate gate;
    WetOverdriveState overdrive;
    int maxBlockSize_ = 0;
    std::vector<float> wetSendScratch_;
    std::vector<float> reverbScratch_;
    std::vector<float> postGateScratch_;
};

} // namespace sendbloom
