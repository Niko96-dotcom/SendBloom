#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

bool bufferAllFinite (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        const auto* samples = buffer.getReadPointer (ch);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            if (! std::isfinite (samples[i]))
                return false;
        }
    }

    return true;
}

} // namespace

TEST_CASE ("v1 unprepared processBlock is safe (RT-15)",
           "[v1][contract][realtime][RT-15]")
{
    // RT-15 / D-05: preparedMaxBlock_ <= 0 must not crash or produce non-finite output.
    sendbloom::PluginProcessor plugin;
    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;

    for (int i = 0; i < kBlockSize; ++i)
    {
        buffer.setSample (0, i, 0.25f);
        buffer.setSample (1, i, -0.15f);
    }

    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
    REQUIRE (bufferAllFinite (buffer));

    plugin.prepareToPlay (kSampleRate, kBlockSize);
    plugin.releaseResources();

    for (int i = 0; i < kBlockSize; ++i)
    {
        buffer.setSample (0, i, 0.25f);
        buffer.setSample (1, i, -0.15f);
    }

    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
    REQUIRE (bufferAllFinite (buffer));
}
