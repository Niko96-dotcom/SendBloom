#pragma once

namespace sendbloom
{

class IReverbEngine
{
public:
    virtual ~IReverbEngine() = default;

    virtual void prepare (double sampleRate, int maxBlockSize) noexcept = 0;

    virtual float processSample (float input,
                                 float rt60Seconds,
                                 float darkMix) noexcept = 0;

    virtual void processBlock (const float* input,
                               float* output,
                               int numSamples,
                               float rt60Seconds,
                               float darkMix) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample (input[i], rt60Seconds, darkMix);
    }

    /** Host-domain latency introduced by the production wet path.

        Implementations that are direct/sample-synchronous keep the default
        zero. The processor uses this value during prepareToPlay only, so it
        can report correct normal PDC and align its direct path without a
        topology change on the audio thread. */
    virtual int getPdcLatencySamples() const noexcept { return 0; }
};

} // namespace sendbloom
