#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kSettleBlocks = 8; // >5 ms @ 48 kHz / 512

void configureBypassPlugin (sendbloom::PluginProcessor& plugin)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.5f;
    // Non-unity output so any OutputStage application breaks per-channel unity.
    *apvts.getRawParameterValue (outputGain) = 6.0f;
    *apvts.getRawParameterValue (bypass) = 1.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (extendedStereo) = 0.0f;
}

} // namespace

TEST_CASE ("v1 settled true bypass is per-channel unity ignoring output gain",
           "[v1][contract][true-bypass][CORE-14]")
{
    // CORE-14..16 / ADR-V1-10: after bypass settle (>5 ms), each output channel
    // matches its input within 1e-6 and ignores Output/Input/Distn/Gate/Level.
    // Current path mono-sums and applies OutputStage.

    sendbloom::PluginProcessor plugin;
    configureBypassPlugin (plugin);
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;

    for (int b = 0; b < kSettleBlocks; ++b)
    {
        buffer.clear();
        plugin.processBlock (buffer, midi);
    }

    REQUIRE_FALSE (plugin.smoothedBank.getBypassWetMixSmoother().isSmoothing());

    constexpr float kLeft = 0.37f;
    constexpr float kRight = -0.21f;

    for (int i = 0; i < kBlockSize; ++i)
    {
        buffer.setSample (0, i, kLeft);
        buffer.setSample (1, i, kRight);
    }

    plugin.processBlock (buffer, midi);

    float maxErrL = 0.0f;
    float maxErrR = 0.0f;

    for (int i = 0; i < kBlockSize; ++i)
    {
        maxErrL = std::max (maxErrL, std::abs (buffer.getSample (0, i) - kLeft));
        maxErrR = std::max (maxErrR, std::abs (buffer.getSample (1, i) - kRight));
    }

    // Intended failure: mono-sum collapse and/or output gain applied.
    REQUIRE (maxErrL < 1.0e-6f);
    REQUIRE (maxErrR < 1.0e-6f);
}
