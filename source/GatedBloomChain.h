#pragma once

#include "EnvelopeDetector.h"
#include "NoiseGate.h"
#include "SchroederTank32.h"
#include "WetOverdrive.h"
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

        wet = StubPressureSend::process (wet, sendGain);
        wet = reverb.processSample (wet, rt60Seconds, darkModeMix, authenticColor);
        wet = WetOverdrive::process (wet, distnBlend);

        if (! gatePreSoft)
            wet *= postGate.process (inputEnvelope, thresholdDb);

        return wet;
    }

private:
    SchroederTank32 reverb;
    EnvelopeDetector envelope;
    NoiseGate preGate;
    NoiseGate postGate;
};

} // namespace sendbloom
