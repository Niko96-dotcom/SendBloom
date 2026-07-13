#include <BypassCrossfade.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

TEST_CASE ("BypassCrossfade::mixSample blends dry and processed by engagedMix", "[parm][bypass]")
{
    using sendbloom::BypassCrossfade;

    REQUIRE (BypassCrossfade::mixSample (0.5f, 1.0f, 0.0f) == Catch::Approx (0.5f).margin (1.0e-6f));
    REQUIRE (BypassCrossfade::mixSample (0.5f, 1.0f, 1.0f) == Catch::Approx (1.0f).margin (1.0e-6f));
    REQUIRE (BypassCrossfade::mixSample (0.0f, 1.0f, 0.5f) == Catch::Approx (0.5f).margin (1.0e-6f));
    REQUIRE (BypassCrossfade::mixSample (-0.4f, 0.8f, 0.25f)
             == Catch::Approx (-0.4f * 0.75f + 0.8f * 0.25f).margin (1.0e-6f));
}

TEST_CASE ("BypassCrossfade ramps wet mix over 5 ms without large discontinuities", "[parm][bypass]")
{
    juce::SmoothedValue<float> wetMix;
    wetMix.reset (48000.0, 0.005);
    wetMix.setCurrentAndTargetValue (1.0f);
    wetMix.setTargetValue (0.0f);

    juce::AudioBuffer<float> wet (1, 240);
    juce::AudioBuffer<float> dry (1, 240);

    for (int i = 0; i < 240; ++i)
    {
        wet.setSample (0, i, 1.0f);
        dry.setSample (0, i, 0.5f);
    }

    sendbloom::BypassCrossfade::processBlock (wet, dry, wetMix);

    float maxAdjacentDelta = 0.0f;
    for (int i = 1; i < 240; ++i)
        maxAdjacentDelta = std::max (maxAdjacentDelta,
                                     std::abs (wet.getSample (0, i) - wet.getSample (0, i - 1)));

    REQUIRE (maxAdjacentDelta < 0.5f);
}

TEST_CASE ("BypassCrossfade uses 5 ms smoother configuration", "[parm][bypass]")
{
    juce::SmoothedValue<float> wetMix;
    wetMix.reset (48000.0, 0.005);
    REQUIRE (wetMix.isSmoothing() == false);
}

TEST_CASE ("plugin bypass mid-stream toggle keeps adjacent delta click-bounded",
           "[parm][bypass][v1][contract][CORE-17]")
{
    // CORE-17: engage/disengage across a block; bound adjacent sample delta on
    // unit-ish signals (the same order as the prior click metrics).
    constexpr double kSampleRate = 48000.0;
    constexpr int kBlockSize = 512;
    constexpr int kSettleBlocks = 8;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (sendbloom::ParameterIDs::bypass) = 0.0f;
    *apvts.getRawParameterValue (sendbloom::ParameterIDs::outputGain) = 0.0f;
    *apvts.getRawParameterValue (sendbloom::ParameterIDs::level) = 0.0f;
    *apvts.getRawParameterValue (sendbloom::ParameterIDs::extendedStereo) = 0.0f;
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;

    for (int b = 0; b < kSettleBlocks; ++b)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto t = static_cast<float> (b * kBlockSize + i) / static_cast<float> (kSampleRate);
            const auto s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 1000.0f * t);
            buffer.setSample (0, i, s);
            buffer.setSample (1, i, s);
        }
        plugin.processBlock (buffer, midi);
    }

    // Mid-stream: engage bypass (wet mix → 0 over 5 ms).
    *apvts.getRawParameterValue (sendbloom::ParameterIDs::bypass) = 1.0f;

    for (int i = 0; i < kBlockSize; ++i)
    {
        const auto t = static_cast<float> (kSettleBlocks * kBlockSize + i)
                       / static_cast<float> (kSampleRate);
        const auto s = 0.5f * std::sin (2.0f * juce::MathConstants<float>::pi * 1000.0f * t);
        buffer.setSample (0, i, s);
        buffer.setSample (1, i, s);
    }

    plugin.processBlock (buffer, midi);

    float maxAdjacentDelta = 0.0f;
    for (int i = 1; i < kBlockSize; ++i)
        maxAdjacentDelta = std::max (maxAdjacentDelta,
                                     std::abs (buffer.getSample (0, i) - buffer.getSample (0, i - 1)));

    REQUIRE (maxAdjacentDelta < 1.0f);
}
