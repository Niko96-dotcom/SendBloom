#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

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

TEST_CASE ("v1 control-rate reverb latch quantum is at most 128 (RT-05)",
           "[v1][contract][realtime][RT-05]")
{
    // ADR-V1-05 / D-04: control-rate RT60/dark/authentic latch in spans ≤ 128.
    REQUIRE (sendbloom::PluginProcessor::kControlQuantum == 128);
    REQUIRE (sendbloom::PluginProcessor::kControlQuantum > 0);

    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.5f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.55f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 1.0f;
    *apvts.getRawParameterValue (inputThreshold) = 0.0f;
    *apvts.getRawParameterValue (extendedStereo) = 1.0f;

    constexpr int kPrepared = 512;
    constexpr int kHostBlock = 2048; // many spans; proves wet continuity across quantum boundaries
    plugin.prepareToPlay (kSampleRate, kPrepared);

    // Mid-ramp size so control-rate values move across spans within one host block.
    *apvts.getRawParameterValue (size) = 0.85f;

    juce::AudioBuffer<float> buffer (2, kHostBlock);
    juce::MidiBuffer midi;
    std::vector<float> dry (static_cast<size_t> (kHostBlock));
    auto phase = 0.0f;

    for (int i = 0; i < kHostBlock; ++i)
    {
        const auto s = 0.28f * std::sin (phase);
        phase += 0.05f;
        dry[static_cast<size_t> (i)] = s;
        buffer.setSample (0, i, s);
        buffer.setSample (1, i, s * 0.85f);
    }

    plugin.processBlock (buffer, midi);
    REQUIRE (bufferAllFinite (buffer));

    double totalWetEnergy = 0.0;

    for (int i = 0; i < kHostBlock; ++i)
    {
        const auto wetApprox = buffer.getSample (0, i) - dry[static_cast<size_t> (i)];
        totalWetEnergy += static_cast<double> (wetApprox) * static_cast<double> (wetApprox);
    }

    REQUIRE (totalWetEnergy > 1.0e-4);

    // Tank / predelay silence occupies early spans; assert wet continuity across the
    // final control-quantum boundary (samples 1792|1920) so span looping is proven.
    const auto q = sendbloom::PluginProcessor::kControlQuantum;
    const auto split = kHostBlock - q; // 1920
    REQUIRE (split >= q);

    double energyBefore = 0.0;
    double energyAfter = 0.0;

    for (int i = 0; i < q; ++i)
    {
        const auto beforeIdx = split - q + i;
        const auto afterIdx = split + i;

        const auto wb = buffer.getSample (0, beforeIdx) - dry[static_cast<size_t> (beforeIdx)];
        const auto wa = buffer.getSample (0, afterIdx) - dry[static_cast<size_t> (afterIdx)];
        energyBefore += static_cast<double> (wb) * static_cast<double> (wb);
        energyAfter += static_cast<double> (wa) * static_cast<double> (wa);
    }

    REQUIRE (energyBefore > 1.0e-4);
    REQUIRE (energyAfter > 1.0e-4);
}
