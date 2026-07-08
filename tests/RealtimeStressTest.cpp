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

TEST_CASE ("processBlock survives 10k varying block stress with authentic color on",
           "[realtime][integration][stress][TEST-09]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (authenticColor) = 1.0f;
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
    *apvts.getRawParameterValue (authenticColor) = 1.0f;
    *apvts.getRawParameterValue (level) = 0.5f;

    juce::AudioBuffer<float> buffer (2, 1024);
    fillNoise (buffer, 0);

    juce::MidiBuffer midi;
    REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            REQUIRE (std::isfinite (buffer.getSample (ch, i)));
}

TEST_CASE ("processBlock survives 1000 authentic_color toggles",
           "[realtime][integration][stress][XFADE-02]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (authenticColor) = 0.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    *apvts.getRawParameterValue (size) = 0.75f;

    juce::MidiBuffer midi;
    float peak = 0.0f;
    int toggleCount = 0;

    for (int toggleIndex = 0; toggleIndex < 1000; ++toggleIndex)
    {
        *apvts.getRawParameterValue (authenticColor) =
            toggleIndex % 2 == 0 ? 1.0f : 0.0f;

        const auto blockSize =
            kBlockSizes[static_cast<size_t> (toggleIndex) % kBlockSizes.size()];
        juce::AudioBuffer<float> buffer (2, blockSize);
        fillNoise (buffer, toggleIndex);

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
        ++toggleCount;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                const auto s = buffer.getSample (ch, i);
                REQUIRE (std::isfinite (s));
                peak = std::max (peak, std::abs (s));
            }
        }

        float maxAdjacentDelta = 0.0f;
        for (int i = 1; i < buffer.getNumSamples(); ++i)
        {
            maxAdjacentDelta = std::max (
                maxAdjacentDelta,
                std::abs (buffer.getSample (0, i) - buffer.getSample (0, i - 1)));
        }
        REQUIRE (maxAdjacentDelta < 1.0f);
    }

    REQUIRE (toggleCount == 1000);
    REQUIRE (peak < 4.0f);
}

TEST_CASE ("processBlock stress with authentic color toggling",
           "[realtime][integration][stress][TEST-09]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (44100.0, 512);

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (authenticColor) = 1.0f;
    *apvts.getRawParameterValue (level) = 0.5f;
    *apvts.getRawParameterValue (size) = 0.75f;

    juce::MidiBuffer midi;

    for (int block = 0; block < 2000; ++block)
    {
        const auto blockSize = kBlockSizes[static_cast<size_t> (block) % kBlockSizes.size()];
        juce::AudioBuffer<float> buffer (2, blockSize);
        fillNoise (buffer, block);

        if (block % 50 == 0)
            *apvts.getRawParameterValue (authenticColor) = (block / 50) % 2 == 0 ? 1.0f : 0.0f;

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
    }
}
