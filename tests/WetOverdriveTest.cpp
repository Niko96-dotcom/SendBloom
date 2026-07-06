#include <WetOverdrive.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

float symmetricTanhDrive (float wet) noexcept
{
    return std::tanh (wet * 3.0f);
}

} // namespace

TEST_CASE ("WetOverdrive passes clean wet at distnBlend zero", "[od][WetOverdrive]")
{
    REQUIRE (sendbloom::WetOverdrive::process (0.35f, 0.0f) == Catch::Approx (0.35f));
    REQUIRE (sendbloom::WetOverdrive::process (-0.2f, 0.0f) == Catch::Approx (-0.2f));
}

TEST_CASE ("WetOverdrive fully drives at distnBlend one", "[od][WetOverdrive]")
{
    const auto input = 0.4f;
    const auto expected = sendbloom::WetOverdrive::asymmetricTanh (input);
    REQUIRE (sendbloom::WetOverdrive::process (input, 1.0f) == Catch::Approx (expected));
}

TEST_CASE ("WetOverdrive asymmetry drives positive harder than negative", "[od][WetOverdrive]")
{
    const auto pos = sendbloom::WetOverdrive::asymmetricTanh (0.25f);
    const auto neg = sendbloom::WetOverdrive::asymmetricTanh (-0.25f);
    const auto symPos = symmetricTanhDrive (0.25f);

    REQUIRE (std::abs (pos) > std::abs (symPos));
    REQUIRE (std::abs (pos) > std::abs (neg));
}

TEST_CASE ("WetOverdrive blend interpolates clean and driven", "[od][WetOverdrive]")
{
    const auto input = 0.3f;
    const auto blend = 0.5f;
    const auto driven = sendbloom::WetOverdrive::asymmetricTanh (input);
    const auto expected = input + blend * (driven - input);

    REQUIRE (sendbloom::WetOverdrive::process (input, blend) == Catch::Approx (expected));
}
