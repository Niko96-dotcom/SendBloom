#pragma once

// Dry unity parallel pedal topology — level knob scales wet return only (CHN-02).

namespace sendbloom
{

struct ParallelWetMixer
{
    static float mix (float dryTap, float wetSample, float wetGain) noexcept
    {
        return dryTap + wetSample * wetGain;
    }
};

} // namespace sendbloom
