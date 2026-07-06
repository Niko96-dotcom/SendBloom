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
};

} // namespace sendbloom
