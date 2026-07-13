#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cmath>

namespace
{

constexpr std::array<int, 8> kBlockSizes { 32, 64, 128, 256, 512, 1024, 256, 128 };

void fillNoise (juce::AudioBuffer<float>& buffer, int blockIndex)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
    {
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto phase = static_cast<float> (blockIndex * 997 + i * 13 + ch * 7);
            buffer.setSample (ch, i, 0.25f * std::sin (phase * 0.017f));
        }
    }
}

} // namespace

TEST_CASE ("processBlock survives 10k varying block stress with fixed ProperSRC",
           "[realtime][integration][stress][TEST-09]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.6f;
    *apvts.getRawParameterValue (distn) = 0.4f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.75f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;

    juce::MidiBuffer midi;
    float peak = 0.0f;

    for (int block = 0; block < 10000; ++block)
    {
        const auto blockSize = kBlockSizes[static_cast<size_t> (block) % kBlockSizes.size()];
        juce::AudioBuffer<float> buffer (2, blockSize);
        fillNoise (buffer, block);

        if (block % 100 == 0)
        {
            const auto toggle = (block / 100) % 2 == 0 ? 0.3f : 0.7f;
            *apvts.getRawParameterValue (level) = toggle;
            *apvts.getRawParameterValue (gatePrePost) = (block / 100) % 3 == 0 ? 1.0f : 0.0f;
        }

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
                peak = std::max (peak, std::abs (s));
            }
        }
    }

    REQUIRE (peak > 0.0f);
    REQUIRE (peak < 4.0f);
}

TEST_CASE ("processBlock survives 10k varying block stress", "[realtime][integration][stress]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    auto& apvts = plugin.getAPVTS();
  *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.6f;
    *apvts.getRawParameterValue (distn) = 0.4f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.75f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;

    juce::MidiBuffer midi;
    float peak = 0.0f;

    for (int block = 0; block < 10000; ++block)
    {
        const auto blockSize = kBlockSizes[static_cast<size_t> (block) % kBlockSizes.size()];
        juce::AudioBuffer<float> buffer (2, blockSize);
        fillNoise (buffer, block);

        if (block % 100 == 0)
        {
            const auto toggle = (block / 100) % 2 == 0 ? 0.3f : 0.7f;
            *apvts.getRawParameterValue (level) = toggle;
            *apvts.getRawParameterValue (gatePrePost) = (block / 100) % 3 == 0 ? 1.0f : 0.0f;
        }

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
                peak = std::max (peak, std::abs (s));
            }
        }
    }

    REQUIRE (peak > 0.0f);
    REQUIRE (peak < 4.0f);
}

TEST_CASE ("processBlock tolerates larger-than-prepared block size",
           "[realtime][integration][regression][TEST-09]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (44100.0, 512);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.5f;

    juce::AudioBuffer<float> buffer (2, 1024);
    fillNoise (buffer, 0);

    juce::MidiBuffer midi;
    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE (std::isfinite (buffer.getSample (ch, i)));
}

TEST_CASE ("10k stress with fixed ProperSRC, bypass, and oversized blocks stays finite (RT-14)",
           "[realtime][integration][stress][RT-14]")
{
    // RT-14 / D-10: churn bypass while occasionally exceeding preparedMaxBlock_.
    using namespace sendbloom::ParameterIDs;

    constexpr int kPrepared = 512;
    constexpr std::array<int, 10> kMixedSizes {
        32, 64, 128, 256, 512, 1024, 2048, 256, 128, 1024
    };

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, kPrepared);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.6f;
    *apvts.getRawParameterValue (distn) = 0.35f;
    *apvts.getRawParameterValue (size) = 0.55f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.8f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;

    juce::MidiBuffer midi;
    float peak = 0.0f;
    int oversizedBlocks = 0;

    for (int block = 0; block < 10000; ++block)
    {
        const auto blockSize = kMixedSizes[static_cast<size_t> (block) % kMixedSizes.size()];
        if (blockSize > kPrepared)
            ++oversizedBlocks;

        juce::AudioBuffer<float> buffer (2, blockSize);
        fillNoise (buffer, block);

        if (block % 60 == 0)
            *apvts.getRawParameterValue (bypass) = (block / 60) % 2 == 0 ? 1.0f : 0.0f;

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
                peak = std::max (peak, std::abs (s));
            }
        }
    }

    REQUIRE (oversizedBlocks > 0);
    REQUIRE (peak > 0.0f);
    REQUIRE (peak < 4.0f);
}
