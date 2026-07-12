#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kPreparedBlock = 512;
constexpr int kOversizedBlock = 2048;

void configureWetPlugin (sendbloom::PluginProcessor& plugin)
{
    using namespace sendbloom::ParameterIDs;

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
}

std::vector<float> fillTone (int numSamples, float phaseStart)
{
    std::vector<float> samples (static_cast<size_t> (numSamples));
    auto phase = phaseStart;

    for (int i = 0; i < numSamples; ++i)
    {
        samples[static_cast<size_t> (i)] = 0.28f * std::sin (phase);
        phase += 0.05f;
    }

    return samples;
}

float maxAbsDiff (const juce::AudioBuffer<float>& a, const juce::AudioBuffer<float>& b)
{
    float maxDiff = 0.0f;
    const auto channels = juce::jmin (a.getNumChannels(), b.getNumChannels());
    const auto samples = juce::jmin (a.getNumSamples(), b.getNumSamples());

    for (int ch = 0; ch < channels; ++ch)
        for (int i = 0; i < samples; ++i)
            maxDiff = std::max (maxDiff, std::abs (a.getSample (ch, i) - b.getSample (ch, i)));

    return maxDiff;
}

float wetEnergyProxy (const juce::AudioBuffer<float>& output, const std::vector<float>& dryInput)
{
    // With level=1 and bypass off, wet contribution is output − dry (extended stereo).
    double energy = 0.0;
    const auto n = juce::jmin (output.getNumSamples(), static_cast<int> (dryInput.size()));

    for (int i = 0; i < n; ++i)
    {
        const auto wetApprox = output.getSample (0, i) - dryInput[static_cast<size_t> (i)];
        energy += static_cast<double> (wetApprox) * static_cast<double> (wetApprox);
    }

    return static_cast<float> (energy);
}

} // namespace

TEST_CASE ("v1 oversized block matches chunked reference with wet continuity",
           "[v1][contract][oversized-block][RT-02]")
{
    // RT-02 / RT-03: prepare(48000, 512) then process one 2048 block must match
    // four×512 reference within 2e-5, and the wet path must be nonzero.
    // Current production falls back to dry-only when numSamples > preparedMaxBlock_.

    sendbloom::PluginProcessor chunked;
    configureWetPlugin (chunked);
    chunked.prepareToPlay (kSampleRate, kPreparedBlock);

    sendbloom::PluginProcessor oneshot;
    configureWetPlugin (oneshot);
    oneshot.prepareToPlay (kSampleRate, kPreparedBlock);

    const auto input = fillTone (kOversizedBlock, 0.0f);

    juce::AudioBuffer<float> reference (2, kOversizedBlock);
    juce::MidiBuffer midi;
    reference.clear();

    for (int chunk = 0; chunk < 4; ++chunk)
    {
        juce::AudioBuffer<float> block (2, kPreparedBlock);
        for (int i = 0; i < kPreparedBlock; ++i)
        {
            const auto s = input[static_cast<size_t> (chunk * kPreparedBlock + i)];
            block.setSample (0, i, s);
            block.setSample (1, i, s * 0.85f);
        }

        chunked.processBlock (block, midi);
        for (int i = 0; i < kPreparedBlock; ++i)
        {
            reference.setSample (0, chunk * kPreparedBlock + i, block.getSample (0, i));
            reference.setSample (1, chunk * kPreparedBlock + i, block.getSample (1, i));
        }
    }

    juce::AudioBuffer<float> oversized (2, kOversizedBlock);
    for (int i = 0; i < kOversizedBlock; ++i)
    {
        oversized.setSample (0, i, input[static_cast<size_t> (i)]);
        oversized.setSample (1, i, input[static_cast<size_t> (i)] * 0.85f);
    }

    oneshot.processBlock (oversized, midi);

    const auto wetEnergy = wetEnergyProxy (oversized, input);
    // Intended failure: oversized dry-fallback forces wet≈0.
    REQUIRE (wetEnergy > 1.0e-4f);

    const auto diff = maxAbsDiff (reference, oversized);
    // Intended failure: dry fallback diverges from chunked wet reference.
    REQUIRE (diff < 2.0e-5f);
}
