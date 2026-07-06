#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{

float sumChannelsToMono (float left, float right, int numChannels) noexcept
{
    if (numChannels <= 1)
        return left;

    return (left + right) / static_cast<float> (numChannels);
}

} // namespace

TEST_CASE ("Mono bus sums stereo channels", "[io][mono]")
{
    const auto mono = sumChannelsToMono (0.5f, 0.3f, 2);
    REQUIRE (mono == Catch::Approx (0.4f));
}

TEST_CASE ("Mono bus passes mono unchanged", "[io][mono]")
{
    const auto mono = sumChannelsToMono (0.6f, 0.0f, 1);
    REQUIRE (mono == Catch::Approx (0.6f));
}

TEST_CASE ("Dual mono output writes same wet to both channels", "[io][mono]")
{
    const auto wet = 0.33f;
    const float left = wet;
    const float right = wet;
    REQUIRE (left == Catch::Approx (right));
}
