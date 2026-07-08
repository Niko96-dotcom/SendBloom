#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>
#include <array>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>

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

std::string extractProcessBlockBody (const std::string& source)
{
    const auto start = source.find ("void PluginProcessor::processBlock");

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

std::string extractProcessBlockBodyFromHeader (const std::string& source)
{
    const auto start = source.find ("void processBlock (");

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

TEST_CASE ("INTEG-04 parameter layout exposes exactly 15 Phase 2 contract IDs",
           "[integrability][INTEG-04]")
{
    using namespace sendbloom::ParameterIDs;

    const std::array<const char*, 15> expected {
        inputGain,
        inputThreshold,
        size,
        level,
        distn,
        outputGain,
        darkMode,
        gatePrePost,
        sendConnected,
        sendAmount,
        sendFeel,
        authenticColor,
        extendedStereo,
        dirtOs,
        bypass,
    };

    REQUIRE (expected.size() == 15);

    for (const auto* id : expected)
    {
        REQUIRE (id != nullptr);
        REQUIRE (std::strlen (id) > 0);
    }

    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getParameters().size() == 15);
}

TEST_CASE ("INTEG-04 authentic_color defaults off in fresh parameter layout",
           "[integrability][INTEG-04]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    const auto* param = plugin.getAPVTS().getParameter (authenticColor);
    REQUIRE (param != nullptr);
    REQUIRE (param->getDefaultValue() == Catch::Approx (0.0f).margin (1e-4f));
    REQUIRE (plugin.getAPVTS().getRawParameterValue (authenticColor)->load()
             == Catch::Approx (0.0f).margin (1e-4f));
}

TEST_CASE ("INTEG-04 ParameterIDs exposes no diagnostics APVTS naming",
           "[integrability][INTEG-04]")
{
    const auto root = findRepoRoot();
    const auto header = readTextFile (root.getChildFile ("source/ParameterIDs.h"));
    REQUIRE_FALSE (header.empty());

    REQUIRE (header.find ("Authentic32Mode") == std::string::npos);
    REQUIRE (header.find ("diagnosticsMode") == std::string::npos);
    REQUIRE (header.find ("LegacyAccumulator") == std::string::npos);
    REQUIRE (header.find ("ProperSRC") == std::string::npos);
    REQUIRE (header.find ("authentic32") == std::string::npos);
}

TEST_CASE ("INTEG-04 block integration keeps authentic_color as sole authentic APVTS route",
           "[integrability][INTEG-04]")
{
    const auto root = findRepoRoot();
    const auto processorSource = readTextFile (root.getChildFile ("source/PluginProcessor.cpp"));
    const auto chainSource = readTextFile (root.getChildFile ("source/GatedBloomChain.h"));
    REQUIRE_FALSE (processorSource.empty());
    REQUIRE_FALSE (chainSource.empty());

    const auto processorBody = extractProcessBlockBody (processorSource);
    const auto chainBody = extractProcessBlockBodyFromHeader (chainSource);
    REQUIRE_FALSE (processorBody.empty());
    REQUIRE_FALSE (chainBody.empty());

    REQUIRE (processorBody.find ("diagnosticsMode") == std::string::npos);
    REQUIRE (processorBody.find ("Authentic32Mode") == std::string::npos);
    REQUIRE (processorBody.find ("setAuthentic32ModeForDiagnostics") == std::string::npos);
    REQUIRE (processorBody.find ("getRawParameterValue (ParameterIDs::authenticColor)") == std::string::npos);

    REQUIRE (chainBody.find ("ParameterIDs") == std::string::npos);
    REQUIRE (chainBody.find ("getRawParameterValue") == std::string::npos);
    REQUIRE (chainBody.find ("diagnosticsMode") == std::string::npos);
    REQUIRE (chainBody.find ("Authentic32Mode") == std::string::npos);

    REQUIRE (processorSource.find ("getNextAuthenticColorTarget") != std::string::npos);
}
