#include "ChainTestHelpers.h"
#include <Fdn8Reverb.h>
#include <GatedBloomChain.h>
#include <ParameterCurves.h>
#include <SchroederTank32.h>
#include <catch2/catch_test_macros.hpp>
#include <memory>
#include <vector>

namespace
{

constexpr auto kSampleRate = 48000.0;

std::vector<float> renderChainImpulse (sendbloom::GatedBloomChain& chain, int numSamples)
{
    const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (0.5f);
    std::vector<float> out (static_cast<size_t> (numSamples), 0.0f);

    for (int i = 0; i < numSamples; ++i)
    {
        const auto in = i == 0 ? 1.0f : 0.0f;
        const auto env = chain.getEnvelope().process (std::abs (in));
        out[static_cast<size_t> (i)] = chain.processSample (in, env, rt60, 0.0f,
                                                             0.0f, 1.0f, true, -40.0f);
    }

    return out;
}

} // namespace

TEST_CASE ("GatedBloomChain defaults to SchroederTank32 engine", "[verb][ReverbEngine][chain]")
{
    sendbloom::GatedBloomChain chain;
    chain.prepare (kSampleRate, 512);

    const auto ir = renderChainImpulse (chain, 8192);
    REQUIRE (sendbloom::test::rms (ir) > 1.0e-4f);
}

TEST_CASE ("GatedBloomChain Fdn8 swap produces different tail than Schroeder", "[verb][ReverbEngine][chain]")
{
    sendbloom::GatedBloomChain schroederChain;
    schroederChain.prepare (kSampleRate, 512);
    const auto schroederIr = renderChainImpulse (schroederChain, 4096);

    sendbloom::GatedBloomChain fdnChain;
    fdnChain.setReverbEngineForTests (std::make_unique<sendbloom::Fdn8Reverb>());
    fdnChain.prepare (kSampleRate, 512);
    const auto fdnIr = renderChainImpulse (fdnChain, 4096);

    double diff = 0.0;

    for (size_t i = 0; i < schroederIr.size(); ++i)
    {
        const auto d = static_cast<double> (schroederIr[i]) - static_cast<double> (fdnIr[i]);
        diff += d * d;
    }

    REQUIRE (diff > 1.0e-6);
    REQUIRE (sendbloom::test::rms (fdnIr) > 1.0e-4f);
}
