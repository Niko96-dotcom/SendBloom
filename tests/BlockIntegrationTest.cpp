#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>

#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <vector>

namespace
{

constexpr auto kThresholdDb = -40.0f;

void fillNoiseBlock (std::vector<float>& mono,
                     std::vector<float>& envelope,
                     sendbloom::GatedBloomChain& chain,
                     int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        mono[static_cast<size_t> (i)] = 0.35f * std::sin (0.03f * static_cast<float> (i));
        envelope[static_cast<size_t> (i)] =
            chain.getEnvelope().process (std::abs (mono[static_cast<size_t> (i)]));
    }
}

void requireFiniteWetOut (const std::vector<float>& wetOut, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
        REQUIRE (std::isfinite (wetOut[static_cast<size_t> (i)]));
}

} // namespace

TEST_CASE ("GatedBloomChain fixed-rate block at 44100 Hz produces finite output",
           "[integration][TEST-09][block]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (44100.0, 512);

    constexpr int kNumSamples = 128;
    std::vector<float> mono (static_cast<size_t> (kNumSamples));
    std::vector<float> envelope (static_cast<size_t> (kNumSamples));
    std::vector<float> wetOut (static_cast<size_t> (kNumSamples));

    fillNoiseBlock (mono, envelope, chain, kNumSamples);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    chain.processBlock (mono.data(), envelope.data(), wetOut.data(), kNumSamples,
                        rt60, 0.0f, 0.3f, 1.0f, true, kThresholdDb);

    requireFiniteWetOut (wetOut, kNumSamples);
}

TEST_CASE ("GatedBloomChain fixed-rate block at 96000 Hz produces finite output",
           "[integration][TEST-09][block]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (96000.0, 512);

    constexpr int kNumSamples = 128;
    std::vector<float> mono (static_cast<size_t> (kNumSamples));
    std::vector<float> envelope (static_cast<size_t> (kNumSamples));
    std::vector<float> wetOut (static_cast<size_t> (kNumSamples));

    fillNoiseBlock (mono, envelope, chain, kNumSamples);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    chain.processBlock (mono.data(), envelope.data(), wetOut.data(), kNumSamples,
                        rt60, 0.0f, 0.3f, 1.0f, true, kThresholdDb);

    requireFiniteWetOut (wetOut, kNumSamples);
}

TEST_CASE ("PluginProcessor fixed-rate reverb block produces finite stereo output",
           "[integration][TEST-09][block]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 256);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    *apvts.getRawParameterValue (distn) = 0.3f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.75f;

    juce::AudioBuffer<float> buffer (2, 256);
    juce::MidiBuffer midi;

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            buffer.setSample (ch, i, 0.25f * std::sin (0.03f * static_cast<float> (i)));

    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE (std::isfinite (buffer.getSample (ch, i)));
}
