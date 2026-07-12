#include <catch2/catch_test_macros.hpp>
#include <juce_core/juce_core.h>
#include <cctype>
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

bool containsIgnoreCase (const std::string& haystack, const std::string& needle)
{
    auto lowerHay = haystack;
    auto lowerNeedle = needle;

    for (auto& c : lowerHay)
        c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));

    for (auto& c : lowerNeedle)
        c = static_cast<char> (std::tolower (static_cast<unsigned char> (c)));

    return lowerHay.find (lowerNeedle) != std::string::npos;
}

} // namespace

TEST_CASE ("v1 shipping policy bans product-facing third-party faceplate naming",
           "[v1][contract][shipping-policy][UX-07]")
{
    // UX-07/08: no product-facing "REVERB X" string; check-legal-metadata.sh does not cover this gap.
    const auto root = findRepoRoot();
    const auto faceplate = readTextFile (root.getChildFile ("source/ui/PedalFaceplatePaint.cpp"));
    REQUIRE_FALSE (faceplate.empty());

    // Intended failure while faceplate still draws the banned product name.
    REQUIRE (faceplate.find ("REVERB X") == std::string::npos);
}

TEST_CASE ("v1 shipping policy bans reverbx shipping resource filenames",
           "[v1][contract][shipping-policy][UX-08]")
{
    const auto root = findRepoRoot();
    const auto cmake = readTextFile (root.getChildFile ("CMakeLists.txt"));
    REQUIRE_FALSE (cmake.empty());

    // Intended failure while juce_add_binary_data still ships reverbx-faceplate.png.
    REQUIRE_FALSE (containsIgnoreCase (cmake, "reverbx"));
}

TEST_CASE ("v1 processBlock bans dryBuffer.setSize and raw sendParam APVTS writes",
           "[v1][contract][shipping-policy][RT-01]")
{
    const auto root = findRepoRoot();
    const auto source = readTextFile (root.getChildFile ("source/PluginProcessor.cpp"));
    REQUIRE_FALSE (source.empty());

    const auto body = extractProcessBlockBody (source);
    REQUIRE_FALSE (body.empty());

    // Intended failures: realtime path still allocates/resizes and mutates APVTS.
    REQUIRE (body.find ("dryBuffer.setSize") == std::string::npos);
    REQUIRE (body.find ("sendParam->store") == std::string::npos);
}
