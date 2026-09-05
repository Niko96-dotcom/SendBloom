#include "ChainTestHelpers.h"
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <fstream>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

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
