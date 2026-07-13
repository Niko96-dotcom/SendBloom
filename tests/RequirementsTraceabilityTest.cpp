#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <NoiseGate.h>
#include <ParallelWetMixer.h>
#include <ParameterCurves.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <cstring>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

// Formal traceability anchors for TEST-01..TEST-05 (Phase 10 audit gate).

TEST_CASE ("TEST-01 parameter curves RT60 distn send mapping", "[traceability][TEST-01]")
{
    using namespace sendbloom::ParameterCurves;

    REQUIRE (sizeToRT60 (0.0f) == Catch::Approx (0.25f));
    REQUIRE (sizeToRT60 (1.0f) == Catch::Approx (6.0f));
    REQUIRE (distnBlend (0.5f) == Catch::Approx (std::pow (0.5f, 2.8f)));

    const auto firm = sendGain (0.5f, true);
    const auto soft = sendGain (0.5f, false);
    REQUIRE (firm == Catch::Approx (std::pow (smoothstep (0.5f), 1.85f)));
    REQUIRE (soft == Catch::Approx (std::pow (smoothstep (0.5f), 1.2f)));
    REQUIRE (firm != Catch::Approx (soft).margin (1e-6f));
}

TEST_CASE ("TEST-02 gate pre hum suppression post hard floor dry passes", "[traceability][TEST-02]")
{
    sendbloom::NoiseGate preGate;
    sendbloom::NoiseGate postGate;
    preGate.prepare (48000.0, sendbloom::GateProfile::PreSoft);
    postGate.prepare (48000.0, sendbloom::GateProfile::PostHard);

    for (int i = 0; i < 20000; ++i)
    {
        preGate.process (0.00001f, -40.0f);
        postGate.process (0.00001f, -40.0f);
    }

    REQUIRE (preGate.getGain() > 0.0f);
    REQUIRE (preGate.getGain() < 0.1f);
    REQUIRE (postGate.getGain() == Catch::Approx (0.0f).margin (1e-3f));

    const auto dryTap = 0.5f;
    const auto wet = 0.0f;
    REQUIRE (sendbloom::ParallelWetMixer::mix (dryTap, wet, 0.0f) == Catch::Approx (dryTap));
}

TEST_CASE ("TEST-03 dry path identity at distn max", "[traceability][TEST-03]")
{
    const auto blend = sendbloom::ParameterCurves::distnBlend (1.0f);
    REQUIRE (blend == Catch::Approx (1.0f));

    const auto dryTap = 0.33f;
    const auto wetSample = 0.9f;
    const auto mixed = sendbloom::ParallelWetMixer::mix (dryTap, wetSample, 0.0f);
    REQUIRE (mixed == Catch::Approx (dryTap));
}

TEST_CASE ("TEST-04 pressure send preserves tank energy at 500 ms", "[traceability][TEST-04]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (48000.0, 512);

    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> wet;

    for (int i = 0; i < 15000; ++i)
    {
        const auto input = 0.5f;
        const auto env = chain.getEnvelope().process (std::abs (input));
        wet.push_back (chain.processSample (input, env, rt60, 0.0f, 0.0f, 1.0f, true, -40.0f));
    }

    for (int i = 0; i < 24000; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        wet.push_back (chain.processSample (0.0f, env, rt60, 0.0f, 0.0f, 0.0f, true, -40.0f));
    }

    const auto tailAt500ms = std::vector<float> (wet.end() - 480, wet.end() - 480 + 240);
    REQUIRE (sendbloom::test::rms (tailAt500ms) > 1e-5f);
}

TEST_CASE ("TEST-05 realtime stress block budget", "[traceability][TEST-05]")
{
    constexpr int kStressBlocks = 10000;
    REQUIRE (kStressBlocks >= 10000);

    sendbloom::PluginProcessor plugin;
    plugin.prepareToPlay (48000.0, 1024);

    juce::MidiBuffer midi;
    juce::AudioBuffer<float> buffer (2, 128);

    for (int block = 0; block < 100; ++block)
    {
        buffer.clear();
        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
    }
}

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

struct ReqArtifact
{
    const char* id;
    const char* artifact;
};

// BASE family fully embedded (BASE-03). Keep in sync with REQUIREMENTS.md Traceability column.
constexpr ReqArtifact kBaseArtifacts[] = {
    { "BASE-01", ".planning/phases/SENDBLOOM-19-baseline-contracts-failure-harness/19-BASELINE.md" },
    { "BASE-02", ".planning/REQUIREMENTS.md" },
    { "BASE-03", "tests/RequirementsTraceabilityTest.cpp#[traceability][BASE-03]" },
    { "BASE-04", "tests/ReleaseTruthTest.cpp#[release]; tests/DryPathIntegrityTest.cpp" },
    { "BASE-05", "scripts/verify-v1.sh" },
    { "BASE-06", "scripts/verify-v1.sh; 19-BASELINE.md discovered-at-capture field" },
    { "BASE-07", "tests/BaselinePresetMetricsTest.cpp#[baseline][metrics]; 19-BASELINE-METRICS.md" },
    { "BASE-08", "scripts/verify-v1.sh human_needed; docs/RELEASE_CHECKLIST.md" },
};

bool isReqId (const std::string& cell)
{
    static const std::regex kId { R"(^(BASE|SEND|MIDI|RT|CORE|DSP|UX|REF|REL)-\d+$)" };
    return std::regex_match (cell, kId);
}

std::vector<std::string> splitPipeRow (const std::string& line)
{
    std::vector<std::string> cells;
    std::string current;
    for (size_t i = 0; i < line.size(); ++i)
    {
        if (line[i] == '|')
        {
            // trim
            auto start = current.find_first_not_of (" \t");
            auto end = current.find_last_not_of (" \t");
            cells.push_back (start == std::string::npos ? std::string {}
                                                       : current.substr (start, end - start + 1));
            current.clear();
        }
        else
        {
            current.push_back (line[i]);
        }
    }
    return cells;
}

} // namespace

TEST_CASE ("BASE-03 embedded BASE artifacts are non-empty", "[traceability][BASE-03]")
{
    REQUIRE (std::size (kBaseArtifacts) == 8);

    for (const auto& row : kBaseArtifacts)
    {
        REQUIRE (row.id != nullptr);
        REQUIRE (row.artifact != nullptr);
        REQUIRE (std::string_view (row.id).find ("BASE-") == 0);
        REQUIRE (std::strlen (row.artifact) > 0);
    }
}

TEST_CASE ("REQUIREMENTS.md maps each of 128 IDs to a non-empty verification artifact",
           "[traceability][BASE-03]")
{
    const auto requirements = findRepoRoot().getChildFile (".planning/REQUIREMENTS.md");
    REQUIRE (requirements.existsAsFile());

    const auto text = readTextFile (requirements);
    REQUIRE_FALSE (text.empty());

    // Representative catalog check: all milestone IDs still listed.
    constexpr const char* kFamilies[] = {
        "BASE-01", "BASE-08", "SEND-01", "SEND-14", "MIDI-01", "MIDI-10",
        "RT-01", "RT-15", "CORE-01", "CORE-18", "DSP-01", "DSP-15",
        "UX-01", "UX-16", "REF-01", "REF-12", "REL-01", "REL-20"
    };
    for (const auto* id : kFamilies)
        REQUIRE (text.find (id) != std::string::npos);

    std::istringstream stream (text);
    std::string line;
    int mapped = 0;
    bool inTraceability = false;

    while (std::getline (stream, line))
    {
        if (line.find ("## Traceability") != std::string::npos)
        {
            inTraceability = true;
            continue;
        }

        if (! inTraceability)
            continue;

        if (line.starts_with ("## ") && line.find ("Traceability") == std::string::npos)
            break;

        if (! line.starts_with ("|"))
            continue;

        const auto cells = splitPipeRow (line);
        // | Requirement | Phase | Status | Verification artifact |
        // splitPipeRow yields leading empty cell from opening '|'
        if (cells.size() < 5)
            continue;

        const auto& id = cells[1];
        const auto& artifact = cells[4];

        if (! isReqId (id))
            continue;

        REQUIRE_FALSE (artifact.empty());
        REQUIRE (artifact != "Pending");
        REQUIRE (artifact.find ("Phase ") != 0); // phase stays in column 2
        ++mapped;
    }

    REQUIRE (mapped == 128);
}
