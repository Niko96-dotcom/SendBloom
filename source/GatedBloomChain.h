#pragma once

#include "EnvelopeDetector.h"
#include "Fdn8Reverb.h"
#include "IReverbEngine.h"
#include "NoiseGate.h"
#include "PressureSend.h"
#include "SchroederTank32.h"
#include "WetOverdrive.h"
#include <memory>

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
    }

    void setReverbEngineForTests (std::unique_ptr<IReverbEngine> engine) noexcept
    {
        reverb = std::move (engine);
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
        wet = WetOverdrive::process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= postGate.process (inputEnvelope, thresholdDb);

        return wet;
    }

private:
    std::unique_ptr<IReverbEngine> reverb;
    EnvelopeDetector envelope;
    NoiseGate preGate;
    NoiseGate postGate;
};

} // namespace sendbloom
