#include <ParameterIDs.h>
#include <catch2/catch_test_macros.hpp>
#include <array>
#include <cstring>

TEST_CASE ("ParameterIDs exposes 15 unique immutable IDs", "[parm][ids]")
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

    for (size_t i = 0; i < expected.size(); ++i)
        for (size_t j = i + 1; j < expected.size(); ++j)
            REQUIRE (std::strcmp (expected[i], expected[j]) != 0);

    REQUIRE (std::strcmp (inputGain, "input_gain") == 0);
    REQUIRE (std::strcmp (bypass, "bypass") == 0);
    REQUIRE (std::strcmp (size, "size") == 0);
}
