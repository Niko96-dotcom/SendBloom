#include <GatedBloomChain.h>
#include <ParallelWetMixer.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE ("Dry path unchanged when gate closes wet", "[gate][DryNever][GatedBloomChain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (48000.0, 512);

    constexpr auto kThresholdDb = -40.0f;
    const auto dryTap = 0.5f;

    for (int i = 0; i < 15000; ++i)
    {
        const auto env = chain.getEnvelope().process (0.5f);
        chain.processSample (0.5f, env, 1.0f, 0.0f, false, 0.0f, 1.0f, false, kThresholdDb);
    }

    for (int i = 0; i < 4800; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        const auto wet = chain.processSample (0.0f, env, 1.0f, 0.0f, false, 0.0f, 1.0f, false, kThresholdDb);
        const auto mixed = sendbloom::ParallelWetMixer::mix (dryTap, wet, 0.0f);
        REQUIRE (mixed == Catch::Approx (dryTap));
    }
}

TEST_CASE ("Pre gate affects wet only not dry tap", "[gate][DryNever][GatedBloomChain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (48000.0, 512);

    constexpr auto kThresholdDb = -40.0f;
    const auto dryTap = 0.4f;

    for (int i = 0; i < 15000; ++i)
    {
        const auto env = chain.getEnvelope().process (0.0f);
        const auto wet = chain.processSample (0.0f, env, 1.0f, 0.0f, false, 0.0f, 1.0f, true, kThresholdDb);
        const auto mixed = sendbloom::ParallelWetMixer::mix (dryTap, wet, 0.5f);
        REQUIRE (mixed == Catch::Approx (dryTap + wet * 0.5f).margin (1e-4f));
    }
}
