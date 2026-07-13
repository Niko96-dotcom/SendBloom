#include "helpers/test_helpers.h"
#include <IReverbEngine.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <memory>

namespace
{

class EnergyTrackingReverb final : public sendbloom::IReverbEngine
{
public:
    void prepare (double, int) noexcept override {}

    float processSample (float input, float, float) noexcept override
    {
        const auto increment = static_cast<double> (std::abs (input));
        auto current = energy.load (std::memory_order_relaxed);

        while (! energy.compare_exchange_weak (current,
                                                current + increment,
                                                std::memory_order_relaxed,
                                                std::memory_order_relaxed))
        {
        }

        return input;
    }

    void resetEnergy() noexcept { energy.store (0.0, std::memory_order_relaxed); }
    double getEnergy() const noexcept { return energy.load (std::memory_order_relaxed); }

private:
    std::atomic<double> energy { 0.0 };
};

void configureSendPlugin (sendbloom::PluginProcessor& plugin, bool connected, float amount = 0.0f)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.5f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = connected ? 1.0f : 0.0f;
    *apvts.getRawParameterValue (sendAmount) = amount;
    plugin.prepareToPlay (48000.0, 512);
}

void fillTone (juce::AudioBuffer<float>& buffer, float amplitude = 0.35f)
{
    auto phase = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto sample = amplitude * std::sin (phase);
        phase += 0.07f;
        buffer.setSample (0, i, sample);

        if (buffer.getNumChannels() > 1)
            buffer.setSample (1, i, sample);
    }
}

} // namespace

TEST_CASE ("MIDI CC1 does not mutate APVTS send_amount when connected", "[midi][send][MIDI-03]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, true, 0.0f);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    fillTone (buffer);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), 0);
    plugin.processBlock (buffer, midi);

    REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendAmount) == Catch::Approx (0.0f).margin (1.0e-6f));
}

TEST_CASE ("MIDI CC1 increases reverb-input energy when connected without APVTS write",
           "[midi][send][MIDI-01]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor quiet;
    configureSendPlugin (quiet, true, 0.0f);
    auto quietTracker = std::make_unique<EnergyTrackingReverb>();
    auto* quietEnergy = quietTracker.get();
    quiet.chain.setReverbEngineForTests (std::move (quietTracker));
    quiet.chain.prepare (48000.0, 512);

    sendbloom::PluginProcessor pressed;
    configureSendPlugin (pressed, true, 0.0f);
    auto pressedTracker = std::make_unique<EnergyTrackingReverb>();
    auto* pressedEnergy = pressedTracker.get();
    pressed.chain.setReverbEngineForTests (std::move (pressedTracker));
    pressed.chain.prepare (48000.0, 512);

    juce::AudioBuffer<float> quietBuf (2, 512);
    juce::AudioBuffer<float> pressedBuf (2, 512);
    juce::MidiBuffer empty;
    juce::MidiBuffer cc1;

    for (int i = 0; i < 64; ++i)
    {
        fillTone (quietBuf);
        fillTone (pressedBuf);
        empty.clear();
        cc1.clear();
        cc1.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
        quiet.processBlock (quietBuf, empty);
        pressed.processBlock (pressedBuf, cc1);
    }

    REQUIRE (*pressed.getAPVTS().getRawParameterValue (sendAmount) == Catch::Approx (0.0f).margin (1.0e-6f));
    REQUIRE (pressedEnergy->getEnergy() > quietEnergy->getEnergy() + 1.0);
}

TEST_CASE ("MIDI CC1 remains modulation-only when send disconnected", "[midi][send][MIDI-01]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor plugin;
    configureSendPlugin (plugin, false);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    fillTone (buffer);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
    plugin.processBlock (buffer, midi);

    REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendAmount) == Catch::Approx (0.0f));
}

TEST_CASE ("Plugin accepts MIDI input", "[midi][send]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.acceptsMidi());
}
