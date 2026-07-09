#include <EngineCrossfade.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{

void fill1kHzSine (float* buffer, int numSamples, float amplitude)
{
    constexpr auto kFreq = 1000.0f;
    constexpr auto kSampleRate = 48000.0f;

    for (int i = 0; i < numSamples; ++i)
        buffer[static_cast<size_t> (i)] =
            amplitude * std::sin (2.0f * juce::MathConstants<float>::pi * kFreq
                                  * static_cast<float> (i) / kSampleRate);
}

float maxAdjacentDelta (const float* buffer, int numSamples)
{
    float maxDelta = 0.0f;

    for (int i = 1; i < numSamples; ++i)
        maxDelta = std::max (maxDelta, std::abs (buffer[static_cast<size_t> (i)]
                                                 - buffer[static_cast<size_t> (i - 1)]));

    return maxDelta;
}

float bufferPeak (const float* buffer, int numSamples)
{
    float peak = 0.0f;

    for (int i = 0; i < numSamples; ++i)
        peak = std::max (peak, std::abs (buffer[static_cast<size_t> (i)]));

    return peak;
}

} // namespace

TEST_CASE ("EngineCrossfade fade duration stays within 20-50 ms", "[verb][EngineCrossfade][XFADE-01]")
{
    sendbloom::EngineCrossfade crossfade;
    crossfade.prepare (48000.0);
    crossfade.beginCrossfadeTowardFixed();

    int sampleCount = 0;
    float host { 0.5f };
    float fixed { 0.5f };
    float out { 0.0f };

    while (crossfade.isCrossfading())
    {
        crossfade.mixWetBlock (&host, &fixed, &out, 1);
        ++sampleCount;
    }

    REQUIRE (sampleCount >= 960);
    REQUIRE (sampleCount <= 2400);
}

TEST_CASE ("EngineCrossfade mid-buffer toggle keeps adjacent delta bounded", "[verb][EngineCrossfade][XFADE-01]")
{
    constexpr int kNumSamples = 512;
    std::vector<float> host (static_cast<size_t> (kNumSamples));
    std::vector<float> fixed (static_cast<size_t> (kNumSamples));
    std::vector<float> output (static_cast<size_t> (kNumSamples), 0.0f);

    fill1kHzSine (host.data(), kNumSamples, 0.5f);
    fill1kHzSine (fixed.data(), kNumSamples, 0.5f);

    sendbloom::EngineCrossfade crossfade;
    crossfade.prepare (48000.0);

    for (int i = 0; i < 256; ++i)
        output[static_cast<size_t> (i)] = host[static_cast<size_t> (i)];

    crossfade.beginCrossfadeTowardFixed();
    crossfade.mixWetBlock (host.data() + 256, fixed.data() + 256, output.data() + 256, 256);

    const auto maxDelta = maxAdjacentDelta (output.data(), kNumSamples);
    REQUIRE (maxDelta < 0.25f);
}

TEST_CASE ("EngineCrossfade normalized click metric stays bounded", "[verb][EngineCrossfade][XFADE-01]")
{
    constexpr int kNumSamples = 512;
    std::vector<float> host (static_cast<size_t> (kNumSamples));
    std::vector<float> fixed (static_cast<size_t> (kNumSamples));
    std::vector<float> output (static_cast<size_t> (kNumSamples), 0.0f);

    fill1kHzSine (host.data(), kNumSamples, 0.5f);
    fill1kHzSine (fixed.data(), kNumSamples, 0.5f);

    sendbloom::EngineCrossfade crossfade;
    crossfade.prepare (48000.0);

    for (int i = 0; i < 256; ++i)
        output[static_cast<size_t> (i)] = host[static_cast<size_t> (i)];

    crossfade.beginCrossfadeTowardFixed();
    crossfade.mixWetBlock (host.data() + 256, fixed.data() + 256, output.data() + 256, 256);

    const auto maxDelta = maxAdjacentDelta (output.data(), kNumSamples);
    const auto peak = bufferPeak (output.data(), kNumSamples);
    const auto normalizedDelta = maxDelta / std::max (peak, 1.0e-6f);

    REQUIRE (normalizedDelta < 0.5f);
}

TEST_CASE ("authentic color crossfade single toggle stays click-bounded", "[integration][XFADE-01]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.75f;
    *apvts.getRawParameterValue (authenticColor) = 0.0f;

    plugin.prepareToPlay (48000.0, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;

    std::vector<float> outputSamples;
    outputSamples.reserve (static_cast<size_t> (4 * 512));

    for (int block = 0; block < 4; ++block)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                buffer.setSample (ch, i, 0.35f * std::sin (0.03f * static_cast<float> (i + block * 512)));

        if (block == 2)
            *apvts.getRawParameterValue (authenticColor) = 1.0f;

        plugin.processBlock (buffer, midi);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
            outputSamples.push_back (buffer.getSample (0, i));
    }

    float maxDelta = 0.0f;

    for (size_t i = 1; i < outputSamples.size(); ++i)
        maxDelta = std::max (maxDelta, std::abs (outputSamples[i] - outputSamples[i - 1]));

    REQUIRE (maxDelta < 1.0f);
}
