#pragma once

#include "PlaceholderReverb.h"
#include "PlaceholderWetDirt.h"
#include "StubInputEnvelope.h"
#include "StubNoiseGate.h"
#include "StubPressureSend.h"

namespace sendbloom
{

class GatedBloomChain
{
public:
    void prepare (double sampleRate, int maxBlockSize) noexcept
    {
        reverb.prepare (sampleRate, maxBlockSize);
        envelope.prepare (sampleRate);
        preGate.prepare (sampleRate, 150.0f, 0.1f);
        postGate.prepare (sampleRate, 7.0f, 0.0f);
    }

    StubInputEnvelope& getEnvelope() noexcept { return envelope; }
    const StubInputEnvelope& getEnvelope() const noexcept { return envelope; }

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
    StubInputEnvelope envelope;
    StubNoiseGate preGate;
    StubNoiseGate postGate;
};

} // namespace sendbloom
