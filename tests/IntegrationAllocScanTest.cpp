#include <catch2/catch_test_macros.hpp>

#include <array>
#include <fstream>
#include <sstream>
#include <string>

namespace
{

std::string readSourceFile (const char* relativePath)
{
    constexpr std::array<const char*, 3> kPathPrefixes {
        "",
        "../",
        "../../",
    };

    for (const auto* prefix : kPathPrefixes)
    {
        std::ostringstream path;
        path << prefix << relativePath;

        std::ifstream in (path.str());
        if (in.is_open())
        {
            std::ostringstream buffer;
            buffer << in.rdbuf();
            return buffer.str();
        }
    }

    return {};
}

std::string stripComments (std::string s)
{
    for (;;)
    {
        const auto block = s.find ("/*");
        if (block == std::string::npos)
            break;

        const auto end = s.find ("*/", block + 2);
        if (end == std::string::npos)
            break;

        s.erase (block, end - block + 2);
    }

    for (;;)
    {
        const auto line = s.find ("//");
        if (line == std::string::npos)
            break;

        const auto end = s.find ('\n', line);
        if (end == std::string::npos)
        {
            s.erase (line);
            break;
        }

        s.erase (line, end - line);
    }

    return s;
}

std::string extractHeaderProcessBlockBody (const std::string& source)
{
    const auto tryExtract = [&source] (const char* signature) -> std::string
    {
        const auto pos = source.find (signature);
        if (pos == std::string::npos)
            return {};

        const auto bodyStart = source.find ('{', pos);
        if (bodyStart == std::string::npos)
            return {};

        const auto nextPublic = source.find ("\npublic:", bodyStart);
        const auto nextPrivate = source.find ("\nprivate:", bodyStart);
        const auto nextProtected = source.find ("\nprotected:", bodyStart);

        auto bodyEnd = source.size();
        for (const auto sectionPos : { nextPublic, nextPrivate, nextProtected })
        {
            if (sectionPos != std::string::npos)
                bodyEnd = std::min (bodyEnd, sectionPos);
        }

        return source.substr (bodyStart, bodyEnd - bodyStart);
    };

    if (auto body = tryExtract ("void processBlock"); ! body.empty())
        return body;

    return tryExtract ("void mixWetBlock");
}

std::string extractCppProcessBlockBody (const std::string& source)
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

std::string extractProcessBlockBody (const std::string& source, const char* path)
{
    const auto isCpp = std::string (path).find (".cpp") != std::string::npos;

    if (isCpp)
        return extractCppProcessBlockBody (source);

    return extractHeaderProcessBlockBody (source);
}

void requireNoAllocTokens (const std::string& strippedBody)
{
    REQUIRE (strippedBody.find ("make_unique") == std::string::npos);
    REQUIRE (strippedBody.find (".resize(") == std::string::npos);
    REQUIRE (strippedBody.find ("push_back") == std::string::npos);
    REQUIRE (strippedBody.find ("emplace_back") == std::string::npos);
    // AudioBuffer capacity grow — banned on the audio thread (RT-01 / D-01).
    // Match ".setSize" so both ".setSize(" and ".setSize (" are rejected.
    REQUIRE (strippedBody.find (".setSize") == std::string::npos);
}

} // namespace

TEST_CASE ("integrated processBlock bodies have no heap allocation tokens",
           "[realtime][TEST-09][static][integration]")
{
    constexpr std::array<const char*, 4> kSources {
        "source/GatedBloomChain.h",
        "source/SchroederTank32.h",
        "source/EngineCrossfade.h",
        "source/PluginProcessor.cpp",
    };

    for (const auto* path : kSources)
    {
        DYNAMIC_SECTION (path)
        {
            const auto source = readSourceFile (path);
            REQUIRE_FALSE (source.empty());

            const auto body = extractProcessBlockBody (source, path);
            REQUIRE_FALSE (body.empty());

            requireNoAllocTokens (stripComments (body));
        }
    }
}
