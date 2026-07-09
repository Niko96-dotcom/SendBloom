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
                                 float darkMix,
                                 bool authenticColor) noexcept = 0;

    virtual void processBlock (const float* input,
                               float* output,
                               int numSamples,
                               float rt60Seconds,
                               float darkMix,
                               bool authenticColor) noexcept
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample (input[i], rt60Seconds, darkMix, authenticColor);
    }

    virtual bool isCrossfading() const noexcept { return false; }

    virtual void requestEngineCrossfade (bool /*targetAuthentic*/) noexcept {}

    virtual int getSrcRoundTripLatencySamples() const noexcept { return 0; }
};

} // namespace sendbloom
