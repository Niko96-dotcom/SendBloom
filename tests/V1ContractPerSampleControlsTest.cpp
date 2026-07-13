#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <fstream>
#include <sstream>
#include <string>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{

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
    return juce::File { SENDBLOOM_SOURCE_DIR };
}

std::string extractProcessSpanBody (const std::string& source)
{
    const auto start = source.find ("void PluginProcessor::processSpan");

    if (start == std::string::npos)
        return {};

    const auto openBrace = source.find ('{', start);

    if (openBrace == std::string::npos)
        return {};

    int depth = 0;

    for (size_t i = openBrace; i < source.size(); ++i)
    {
        if (source[i] == '{')
            ++depth;
        else if (source[i] == '}')
        {
            --depth;

            if (depth == 0)
                return source.substr (openBrace, i - openBrace + 1);
        }
    }

    return {};
}

} // namespace

TEST_CASE ("v1 GatedBloomChain consumes per-sample send/distn/threshold arrays",
           "[v1][contract][per-sample][RT-06]")
{
    constexpr int kN = 128;
    constexpr float kThresholdDb = -40.0f;
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);

    sendbloom::GatedBloomChain chain;
    chain.prepare (48000.0, 512);

    std::vector<float> mono (kN, 0.0f);
    std::vector<float> env (kN, 0.0f);
    std::vector<float> sendVarying (kN, 0.0f);
    std::vector<float> sendUnity (kN, 1.0f);
    std::vector<float> distn (kN, 0.0f);
    std::vector<float> distnVarying (kN, 0.0f);
    std::vector<float> thresh (kN, kThresholdDb);
    std::vector<float> outSilent (kN, 0.0f);
    std::vector<float> outUnity (kN, 0.0f);
    std::vector<float> outDirty (kN, 0.0f);

    for (int i = 0; i < kN; ++i)
    {
        mono[static_cast<size_t> (i)] = 0.35f * std::sin (0.03f * static_cast<float> (i));
        env[static_cast<size_t> (i)] = chain.getEnvelope().process (std::abs (mono[static_cast<size_t> (i)]));
        sendVarying[static_cast<size_t> (i)] = (i < kN / 2) ? 0.0f : 1.0f;
        distnVarying[static_cast<size_t> (i)] = (i < kN / 2) ? 0.0f : 1.0f;
    }

    for (int i = 0; i < 2000; ++i)
    {
        const auto e = chain.getEnvelope().process (0.5f);
        chain.processSample (0.5f, e, rt60, 0.0f, 0.0f, 1.0f, true, kThresholdDb);
    }

    chain.processBlock (mono.data(), env.data(), outUnity.data(), kN,
                        rt60, 0.0f, distn.data(), sendUnity.data(), thresh.data(), true);

    double unityEnergy = 0.0;

    for (int i = 0; i < kN; ++i)
        unityEnergy += std::abs (outUnity[static_cast<size_t> (i)]);

    REQUIRE (unityEnergy > 1.0e-3);

    chain.processBlock (mono.data(), env.data(), outSilent.data(), kN,
                        rt60, 0.0f, distn.data(), sendVarying.data(), thresh.data(), true);

    double early = 0.0;
    double late = 0.0;

    for (int i = 0; i < kN / 2; ++i)
        early += std::abs (outSilent[static_cast<size_t> (i)]);

    for (int i = kN / 2; i < kN; ++i)
        late += std::abs (outSilent[static_cast<size_t> (i)]);

    REQUIRE (late > early);
    REQUIRE (late > 1.0e-3);

    chain.processBlock (mono.data(), env.data(), outDirty.data(), kN,
                        rt60, 0.0f, distnVarying.data(), sendUnity.data(), thresh.data(), true);

    double dirtyDelta = 0.0;

    for (int i = kN / 2; i < kN; ++i)
        dirtyDelta += std::abs (outDirty[static_cast<size_t> (i)] - outUnity[static_cast<size_t> (i)]);

    REQUIRE (dirtyDelta > 1.0e-4);
}

TEST_CASE ("v1 processSpan fills per-sample control scratches for RT-06/07",
           "[v1][contract][per-sample][RT-06][RT-07][source-scan]")
{
    const auto root = findRepoRoot();
    const auto source = readTextFile (root.getChildFile ("source/PluginProcessor.cpp"));
    REQUIRE_FALSE (source.empty());

    const auto body = extractProcessSpanBody (source);
    REQUIRE_FALSE (body.empty());

    REQUIRE (body.find ("sendGainScratch_") != std::string::npos);
    REQUIRE (body.find ("distnScratch_") != std::string::npos);
    REQUIRE (body.find ("thresholdDbScratch_") != std::string::npos);
    REQUIRE (body.find ("wetGainScratch_") != std::string::npos);
    REQUIRE (body.find ("outputGainScratch_") != std::string::npos);
    REQUIRE (body.find ("bypassWetScratch_") != std::string::npos);
    REQUIRE (body.find ("getNextInputGainLinear") != std::string::npos);

    REQUIRE (body.find ("distnScratch_.data()") != std::string::npos);
    REQUIRE (body.find ("thresholdDbScratch_.data()") != std::string::npos);
    REQUIRE (body.find ("sendGainScratch_.data()") != std::string::npos);
}

TEST_CASE ("v1 wet level scales wet contribution (RT-07 sample-resolved path)",
           "[v1][contract][per-sample][RT-07]")
{
    auto wetDeltaEnergy = [] (float levelNorm) -> double
    {
        using namespace sendbloom::ParameterIDs;

        sendbloom::PluginProcessor plugin;
        auto& apvts = plugin.getAPVTS();
        *apvts.getRawParameterValue (inputGain) = 1.0f;
        *apvts.getRawParameterValue (outputGain) = 0.0f;
        *apvts.getRawParameterValue (bypass) = 0.0f;
        *apvts.getRawParameterValue (level) = levelNorm;
        *apvts.getRawParameterValue (distn) = 0.0f;
        *apvts.getRawParameterValue (size) = 0.5f;
        *apvts.getRawParameterValue (gatePrePost) = 0.0f;
        *apvts.getRawParameterValue (sendConnected) = 0.0f;
        plugin.prepareToPlay (48000.0, 256);
        plugin.smoothedBank.snapToTargets();

        juce::AudioBuffer<float> buffer (2, 256);
        juce::MidiBuffer midi;
        auto phase = 0.0f;
        double energy = 0.0;

        for (int b = 0; b < 16; ++b)
        {
            for (int i = 0; i < 256; ++i)
            {
                const auto s = 0.35f * std::sin (phase);
                phase += 0.05f;
                buffer.setSample (0, i, s);
                buffer.setSample (1, i, s);
            }

            // Mono-first engaged path uses mono dry; isolate wet via output - dryTap.
            std::vector<float> dryL (256), dryR (256);

            for (int i = 0; i < 256; ++i)
            {
                dryL[static_cast<size_t> (i)] = buffer.getSample (0, i);
                dryR[static_cast<size_t> (i)] = buffer.getSample (1, i);
            }

            plugin.processBlock (buffer, midi);

            for (int i = 0; i < 256; ++i)
            {
                energy += std::abs (buffer.getSample (0, i) - dryL[static_cast<size_t> (i)]);
                energy += std::abs (buffer.getSample (1, i) - dryR[static_cast<size_t> (i)]);
            }
        }

        return energy;
    };

    const auto low = wetDeltaEnergy (0.1f);
    const auto high = wetDeltaEnergy (1.0f);
    REQUIRE (high > low * 1.5);
    REQUIRE (std::isfinite (high));
}
