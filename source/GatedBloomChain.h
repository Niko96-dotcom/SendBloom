#pragma once

#include "EnvelopeDetector.h"
#include "NoiseGate.h"
#include "PlaceholderReverb.h"
#include "PlaceholderWetDirt.h"
#include "StubPressureSend.h"

namespace sendbloom
{

class GatedBloomChain
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        reverb.prepare (sampleRate, maxBlockSize);
        envelope.prepare (sampleRate, 5.0f, 10.0f);
        preGate.prepare (sampleRate, GateProfile::PreSoft);
        postGate.prepare (sampleRate, GateProfile::PostHard);
    }

    EnvelopeDetector& getEnvelope() noexcept { return envelope; }
    const EnvelopeDetector& getEnvelope() const noexcept { return envelope; }

    float processSample (float input,
                         float inputEnvelope,
                         float rt60Seconds,
                         float distnBlend,
                         float sendGain,
                         bool gatePreSoft,
                         float thresholdDb) noexcept
    {
        auto wet = input;

        if (gatePreSoft)
            wet *= preGate.process (inputEnvelope, thresholdDb);

        wet = StubPressureSend::process (wet, sendGain);
        wet = reverb.processSample (wet, rt60Seconds);
        wet = PlaceholderWetDirt::process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= postGate.process (inputEnvelope, thresholdDb);

        return wet;
    }

private:
    PlaceholderReverb reverb;
    EnvelopeDetector envelope;
    NoiseGate preGate;
    NoiseGate postGate;
};

} // namespace sendbloom
