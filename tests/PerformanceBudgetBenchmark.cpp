#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>

#include <array>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <iostream>
#include <memory>
#include <vector>

namespace
{

void configureBenchmarkInstance (sendbloom::PluginProcessor& plugin,
                                 double sampleRate,
                                 int blockSize)
{
    using namespace sendbloom::ParameterIDs;
    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.6f;
    *apvts.getRawParameterValue (inputThreshold) = 0.5f;
    *apvts.getRawParameterValue (size) = 0.65f;
    *apvts.getRawParameterValue (level) = 0.7f;
    *apvts.getRawParameterValue (distn) = 0.35f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (darkMode) = 1.0f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    plugin.prepareToPlay (sampleRate, blockSize);
}

void fillBenchmarkInput (juce::AudioBuffer<float>& buffer)
{
    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto value = 0.2f * std::sin (0.017f * static_cast<float> (sample));
        buffer.setSample (0, sample, value);
        buffer.setSample (1, sample, value);
    }
}

} // namespace

TEST_CASE ("Fixed 32 kHz production engine CPU budget matrix", "[.performance-budget]")
{
    constexpr std::array<int, 4> instanceCounts { 1, 8, 16, 32 };
    constexpr std::array<double, 4> sampleRates { 44100.0, 48000.0, 96000.0, 192000.0 };
    constexpr std::array<int, 5> blockSizes { 32, 64, 128, 512, 1024 };
    constexpr auto measuredAudioSeconds = 0.1;
    constexpr int measurementRepeats = 3;

    double checksum = 0.0;

    for (const auto sampleRate : sampleRates)
    {
        for (const auto blockSize : blockSizes)
        {
            const auto blocks = std::max (
                1, static_cast<int> (std::ceil (measuredAudioSeconds * sampleRate
                                                / static_cast<double> (blockSize))));
            const auto renderedSeconds = static_cast<double> (blocks * blockSize) / sampleRate;

            for (const auto instanceCount : instanceCounts)
            {
                std::vector<std::unique_ptr<sendbloom::PluginProcessor>> plugins;
                std::vector<juce::AudioBuffer<float>> buffers;
                plugins.reserve (static_cast<size_t> (instanceCount));
                buffers.reserve (static_cast<size_t> (instanceCount));

                for (int instance = 0; instance < instanceCount; ++instance)
                {
                    auto plugin = std::make_unique<sendbloom::PluginProcessor>();
                    configureBenchmarkInstance (*plugin, sampleRate, blockSize);
                    plugins.push_back (std::move (plugin));
                    buffers.emplace_back (2, blockSize);
                    fillBenchmarkInput (buffers.back());
                }

                juce::MidiBuffer midi;
                for (int warmup = 0; warmup < 2; ++warmup)
                    for (int instance = 0; instance < instanceCount; ++instance)
                        plugins[static_cast<size_t> (instance)]->processBlock (
                            buffers[static_cast<size_t> (instance)], midi);

                std::array<double, measurementRepeats> measurements {};
                for (auto& measurement : measurements)
                {
                    const auto start = std::chrono::steady_clock::now();
                    for (int block = 0; block < blocks; ++block)
                        for (int instance = 0; instance < instanceCount; ++instance)
                            plugins[static_cast<size_t> (instance)]->processBlock (
                                buffers[static_cast<size_t> (instance)], midi);
                    const auto elapsed = std::chrono::duration<double> (
                        std::chrono::steady_clock::now() - start).count();
                    measurement = elapsed / renderedSeconds * 100.0;
                }
                std::sort (measurements.begin(), measurements.end());
                const auto singleCorePercent = measurements[1];

                for (const auto& buffer : buffers)
                    checksum += std::abs (static_cast<double> (
                        buffer.getSample (0, buffer.getNumSamples() - 1)));

                std::cout << "PERF\tengine=fixed32"
                          << "\tinstances=" << instanceCount
                          << "\trate=" << static_cast<int> (sampleRate)
                          << "\tblock=" << blockSize
                          << "\tmedian_single_core_percent=" << singleCorePercent
                          << "\tworst_single_core_percent=" << measurements.back()
                          << '\n';

                REQUIRE (std::isfinite (singleCorePercent));
                REQUIRE (singleCorePercent >= 0.0);
            }
        }
    }

    REQUIRE (std::isfinite (checksum));
}
