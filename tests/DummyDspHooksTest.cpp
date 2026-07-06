#include <DummyDspHooks.h>
#include <juce_audio_basics/juce_audio_basics.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>

namespace
{

float rms (const juce::AudioBuffer<float>& buffer)
{
    double sum = 0.0;
    const auto n = buffer.getNumSamples();

    for (int i = 0; i < n; ++i)
    {
        const auto s = buffer.getSample (0, i);
        sum += static_cast<double> (s) * static_cast<double> (s);
    }

    return static_cast<float> (std::sqrt (sum / static_cast<double> (n)));
}

void renderDummy (juce::AudioBuffer<float>& buffer, float distnBlend)
{
    sendbloom::DummyDspState state;
    state.prepare (48000.0);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto input = std::sin (0.05f * static_cast<float> (i));
        buffer.setSample (0, i,
                          sendbloom::DummyDspHooks::processSample (input, 0.5f, distnBlend, 1.0f, 1.0f,
                                                                   0.0f, state, 0));
    }
}

} // namespace

TEST_CASE ("DummyDspHooks distn changes output RMS", "[parm][dummy]")
{
    juce::AudioBuffer<float> clean (1, 512);
    juce::AudioBuffer<float> driven (1, 512);

    renderDummy (clean, 0.0f);
    renderDummy (driven, 1.0f);

    REQUIRE (rms (driven) != Catch::Approx (rms (clean)).margin (1e-4f));
}
