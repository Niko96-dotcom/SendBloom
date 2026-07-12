#include <FactoryPresets.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

namespace
{

constexpr auto kSampleRate = 48000.0;
constexpr int kBlockSize = 512;
constexpr int kNumBlocks = 24;
constexpr float kSinePhaseInc = 0.03f;

float bufferPeak (const juce::AudioBuffer<float>& buffer, int channel)
{
    float peak = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
        peak = std::max (peak, std::abs (buffer.getSample (channel, i)));

    return peak;
}

std::string readTextFile (const juce::File& file)
{
    std::ifstream stream (file.getFullPathName().toStdString());

    if (! stream)
        return {};

    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

juce::File findRepoRoot()
{
    auto dir = juce::File::getCurrentWorkingDirectory();

    for (int depth = 0; depth < 8; ++depth)
    {
        const auto cmakeLists = dir.getChildFile ("CMakeLists.txt");

        if (cmakeLists.existsAsFile())
        {
            const auto cmakeText = readTextFile (cmakeLists);

            if (cmakeText.find ("SendBloom") != std::string::npos)
                return dir;
        }

        dir = dir.getParentDirectory();
    }

    return juce::File::getCurrentWorkingDirectory();
}

struct PresetMetrics
{
    juce::String name;
    float peakL = 0.0f;
    float peakR = 0.0f;
    float rmsL = 0.0f;
    float rmsR = 0.0f;
};

PresetMetrics renderPresetMetrics (int presetIndex)
{
    sendbloom::PluginProcessor plugin;
    plugin.setCurrentProgram (presetIndex);
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;
    float peakL = 0.0f;
    float peakR = 0.0f;
    double sumSqL = 0.0;
    double sumSqR = 0.0;
    int sampleCount = 0;

    for (int block = 0; block < kNumBlocks; ++block)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto sample = 0.2f * std::sin (kSinePhaseInc * static_cast<float> (block * kBlockSize + i));
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        plugin.processBlock (buffer, midi);

        peakL = std::max (peakL, bufferPeak (buffer, 0));
        peakR = std::max (peakR, bufferPeak (buffer, 1));

        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto l = buffer.getSample (0, i);
            const auto r = buffer.getSample (1, i);
            sumSqL += static_cast<double> (l) * static_cast<double> (l);
            sumSqR += static_cast<double> (r) * static_cast<double> (r);
        }

        sampleCount += kBlockSize;
    }

    plugin.releaseResources();

    PresetMetrics metrics;
    metrics.name = sendbloom::FactoryPresets::getPresetName (presetIndex);
    metrics.peakL = peakL;
    metrics.peakR = peakR;
    metrics.rmsL = static_cast<float> (std::sqrt (sumSqL / static_cast<double> (sampleCount)));
    metrics.rmsR = static_cast<float> (std::sqrt (sumSqR / static_cast<double> (sampleCount)));
    return metrics;
}

} // namespace

TEST_CASE ("Factory preset baseline peak/RMS metrics are finite and documented",
           "[baseline][metrics]")
{
    const auto repoRoot = findRepoRoot();
    const auto metricsDoc = repoRoot.getChildFile (
        ".planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE-METRICS.md");

    // RED gate: committed metrics artifact must exist before this suite is green.
    REQUIRE (metricsDoc.existsAsFile());

    const auto metricsText = readTextFile (metricsDoc);
    REQUIRE_FALSE (metricsText.empty());
    REQUIRE (metricsText.find ("48 kHz") != std::string::npos);
    REQUIRE (metricsText.find ("512") != std::string::npos);
    REQUIRE (metricsText.find ("[baseline][metrics]") != std::string::npos);

    REQUIRE (sendbloom::FactoryPresets::kNumPresets == 8);

    for (int index = 0; index < sendbloom::FactoryPresets::kNumPresets; ++index)
    {
        const auto metrics = renderPresetMetrics (index);

        REQUIRE (std::isfinite (metrics.peakL));
        REQUIRE (std::isfinite (metrics.peakR));
        REQUIRE (std::isfinite (metrics.rmsL));
        REQUIRE (std::isfinite (metrics.rmsR));
        REQUIRE (metrics.peakL >= 0.0f);
        REQUIRE (metrics.peakR >= 0.0f);
        REQUIRE (metrics.rmsL >= 0.0f);
        REQUIRE (metrics.rmsR >= 0.0f);

        REQUIRE (metricsText.find (metrics.name.toStdString()) != std::string::npos);

        // Always print so 19-BASELINE-METRICS.md can be regenerated from the same path.
        std::cout << "BASELINE_METRICS\t" << metrics.name
                  << "\tpeakL=" << metrics.peakL
                  << "\tpeakR=" << metrics.peakR
                  << "\trmsL=" << metrics.rmsL
                  << "\trmsR=" << metrics.rmsR
                  << '\n';
    }
}
