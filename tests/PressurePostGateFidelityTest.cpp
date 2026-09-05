#include <PluginProcessor.h>
#include <ParameterIDs.h>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <vector>

namespace
{
double releaseResidual (double rate, int block, bool post, bool soft, bool midiControl)
{
    using namespace sendbloom::ParameterIDs;
    sendbloom::PluginProcessor plugin;
    auto& state = plugin.getAPVTS();
    *state.getRawParameterValue (inputGain) = .5f;
    *state.getRawParameterValue (outputGain) = 0.f;
    *state.getRawParameterValue (level) = 1.f;
    *state.getRawParameterValue (size) = 1.f;
    *state.getRawParameterValue (distn) = .75f;
    *state.getRawParameterValue (gatePrePost) = post ? 1.f : 0.f;
    *state.getRawParameterValue (sendConnected) = 1.f;
    *state.getRawParameterValue (sendAmount) = midiControl ? 0.f : 1.f;
    *state.getRawParameterValue (sendFeel) = soft ? 1.f : 0.f;
    plugin.prepareToPlay (rate, block);
    const int latency = plugin.getLatencySamples();
    const int release = static_cast<int> (rate * .75);
    const int total = static_cast<int> (rate * 1.5);
    std::vector<float> input (static_cast<size_t> (total));
    for (int i = 0; i < total; ++i)
        input[static_cast<size_t> (i)] = .25f * static_cast<float> (
            std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * i / rate));
    juce::AudioBuffer<float> buffer (2, block);
    juce::MidiBuffer midi;
    double energy = 0;
    int measured = 0;
    for (int start = 0; start < total;)
    {
        int count = std::min (block, total - start);
        // Host-parameter case starts a callback exactly at release; MIDI uses an
        // event inside the existing block and exercises the processor's span path.
        if (! midiControl && start < release)
            count = std::min (count, release - start);
        if (! midiControl && start == release)
            *state.getRawParameterValue (sendAmount) = 0.f;
        buffer.setSize (2, count, false, false, true);
        for (int ch = 0; ch < 2; ++ch)
            buffer.copyFrom (ch, 0, input.data() + start, count);
        midi.clear();
        if (midiControl && start == 0)
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
        if (midiControl && start <= release && start + count > release)
            midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 0), release - start);
        plugin.processBlock (buffer, midi);
        for (int i = 0; i < count; ++i)
        {
            const int at = start + i;
            // Allow the existing 25ms pressure smoother to settle. This 200ms
            // bound is a software regression criterion, not measured pedal time.
            if (at < release + static_cast<int> (.2 * rate))
                continue;
            const float dry = input[static_cast<size_t> (at - latency)];
            const double wet = buffer.getSample (0, i) - dry;
            energy += wet * wet;
            ++measured;
            REQUIRE (buffer.getSample (0, i) == buffer.getSample (1, i));
        }
        start += count;
    }
    REQUIRE (measured > 0);
    return std::sqrt (energy / measured);
}
}

TEST_CASE ("Pressure release cuts post-gated reverb while dry playing continues",
           "[fidelity][pressure-post-gate]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    for (double rate : { 44100.0, 48000.0, 96000.0 })
        for (int block : { 64, 511 })
            for (bool soft : { false, true })
                for (bool midi : { false, true })
                {
                    INFO ("rate=" << rate << " block=" << block << " soft=" << soft << " midi=" << midi);
                    const auto pre = releaseResidual (rate, block, false, soft, midi);
                    const auto post = releaseResidual (rate, block, true, soft, midi);
                    REQUIRE (pre > 1.e-4); // Pre preserves the already-excited tank.
                    REQUIRE (post < 1.e-7); // Post is dry despite continued input.
                }
}
