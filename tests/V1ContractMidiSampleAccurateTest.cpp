#include <IReverbEngine.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

namespace
{

constexpr double kSampleRate = 48000.0;

/** Records absolute energy at reverb input and per-sample feed magnitudes. */
class EnergyTrackingReverb final : public sendbloom::IReverbEngine
{
public:
    void prepare (double, int maxBlock) noexcept override
    {
        perSampleAbs.assign (static_cast<size_t> (juce::jmax (1, maxBlock)), 0.0f);
    }

    float processSample (float input, float, float) noexcept override
    {
        const auto idx = sampleIndex++;
        energy.fetch_add (static_cast<double> (std::abs (input)), std::memory_order_relaxed);

        if (idx >= 0 && static_cast<size_t> (idx) < perSampleAbs.size())
            perSampleAbs[static_cast<size_t> (idx)] = std::abs (input);

        return input;
    }

    void processBlock (const float* input, float* output, int numSamples,
                       float, float) noexcept override
    {
        for (int i = 0; i < numSamples; ++i)
            output[i] = processSample (input[i], 0.0f, 0.0f);
    }

    void resetEnergy() noexcept
    {
        energy.store (0.0, std::memory_order_relaxed);
        sampleIndex = 0;
        std::fill (perSampleAbs.begin(), perSampleAbs.end(), 0.0f);
    }

    void beginBlock (int numSamples) noexcept
    {
        sampleIndex = 0;

        if (static_cast<int> (perSampleAbs.size()) < numSamples)
            perSampleAbs.assign (static_cast<size_t> (numSamples), 0.0f);
        else
            std::fill (perSampleAbs.begin(), perSampleAbs.begin() + numSamples, 0.0f);
    }

    double getEnergy() const noexcept { return energy.load (std::memory_order_relaxed); }

    float absAt (int sample) const noexcept
    {
        if (sample < 0 || static_cast<size_t> (sample) >= perSampleAbs.size())
            return 0.0f;

        return perSampleAbs[static_cast<size_t> (sample)];
    }

    std::vector<float> perSampleAbs;

private:
    std::atomic<double> energy { 0.0 };
    int sampleIndex { 0 };
};

void configureConnectedRest (sendbloom::PluginProcessor& plugin, int blockSize)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 1.0f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.35f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 1.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.0f;
    *apvts.getRawParameterValue (inputThreshold) = 0.0f;
    plugin.prepareToPlay (kSampleRate, blockSize);
}

void fillTone (juce::AudioBuffer<float>& buffer, float amplitude = 0.4f)
{
    auto phase = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        const auto s = amplitude * std::sin (phase);
        phase += 0.09f;
        buffer.setSample (0, i, s);
        buffer.setSample (1, i, s);
    }
}

bool isFiniteBuffer (const juce::AudioBuffer<float>& buffer)
{
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
            if (! std::isfinite (buffer.getSample (ch, i)))
                return false;

    return true;
}

} // namespace

TEST_CASE ("v1 MIDI CC1 sample position gates pressure feed",
           "[v1][contract][midi][MIDI-04][RT-04]")
{
    sendbloom::PluginProcessor plugin;
    configureConnectedRest (plugin, 512);

    auto tracker = std::make_unique<EnergyTrackingReverb>();
    auto* raw = tracker.get();
    plugin.chain.setReverbEngineForTests (std::move (tracker));
    plugin.chain.prepare (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    fillTone (buffer);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 256);

    raw->beginBlock (512);
    plugin.processBlock (buffer, midi);

    double early = 0.0;
    double late = 0.0;

    for (int i = 0; i < 256; ++i)
        early += static_cast<double> (raw->absAt (i));

    for (int i = 256; i < 512; ++i)
        late += static_cast<double> (raw->absAt (i));

    REQUIRE (early < 1.0e-3);
    REQUIRE (late > early + 1.0);
    REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::sendAmount)
             == Catch::Approx (0.0f).margin (1.0e-6f));
}

TEST_CASE ("v1 MIDI CC1 ordered events create bounded send window",
           "[v1][contract][midi][MIDI-05][MIDI-06]")
{
    sendbloom::PluginProcessor plugin;
    configureConnectedRest (plugin, 512);

    auto tracker = std::make_unique<EnergyTrackingReverb>();
    auto* raw = tracker.get();
    plugin.chain.setReverbEngineForTests (std::move (tracker));
    plugin.chain.prepare (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    fillTone (buffer);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 128);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 0), 384);

    raw->beginBlock (512);
    plugin.processBlock (buffer, midi);

    double before = 0.0;
    double middle = 0.0;

    for (int i = 0; i < 128; ++i)
        before += static_cast<double> (raw->absAt (i));

    for (int i = 128; i < 384; ++i)
        middle += static_cast<double> (raw->absAt (i));

    REQUIRE (before < 1.0e-3);
    REQUIRE (middle > before + 1.0);

    // After CC1=0, sustain release for ~25 ms then prove feed returns near dry.
    juce::MidiBuffer releaseMidi;
    raw->resetEnergy();

    for (int b = 0; b < 40; ++b)
    {
        fillTone (buffer);
        releaseMidi.clear();
        plugin.processBlock (buffer, releaseMidi);
    }

    raw->resetEnergy();
    fillTone (buffer);
    plugin.processBlock (buffer, releaseMidi);
    REQUIRE (raw->getEnergy() < 1.0e-3);
}

TEST_CASE ("v1 non-CC1 MIDI does not change pressure",
           "[v1][contract][midi][MIDI-09]")
{
    sendbloom::PluginProcessor plugin;
    configureConnectedRest (plugin, 512);

    auto tracker = std::make_unique<EnergyTrackingReverb>();
    auto* raw = tracker.get();
    plugin.chain.setReverbEngineForTests (std::move (tracker));
    plugin.chain.prepare (kSampleRate, 512);

    juce::AudioBuffer<float> buffer (2, 512);
    juce::MidiBuffer midi;
    fillTone (buffer);
    midi.addEvent (juce::MidiMessage::controllerEvent (1, 2, 127), 0);
    midi.addEvent (juce::MidiMessage::noteOn (1, 60, (juce::uint8) 100), 10);

    raw->beginBlock (512);
    plugin.processBlock (buffer, midi);

    REQUIRE (raw->getEnergy() < 1.0e-3);
}

TEST_CASE ("v1 host and MIDI pressure combine via max",
           "[v1][contract][midi][MIDI-07]")
{
    using namespace sendbloom::ParameterIDs;

    sendbloom::PluginProcessor hostOnly;
    configureConnectedRest (hostOnly, 256);
    *hostOnly.getAPVTS().getRawParameterValue (sendAmount) = 0.8f;
    hostOnly.prepareToPlay (kSampleRate, 256);

    sendbloom::PluginProcessor midiOnly;
    configureConnectedRest (midiOnly, 256);

    sendbloom::PluginProcessor combined;
    configureConnectedRest (combined, 256);
    *combined.getAPVTS().getRawParameterValue (sendAmount) = 0.3f;
    combined.prepareToPlay (kSampleRate, 256);

    juce::AudioBuffer<float> a (2, 256), b (2, 256), c (2, 256);
    juce::MidiBuffer empty, cc1;
    double energyHost = 0.0, energyMidi = 0.0, energyCombined = 0.0;

    for (int i = 0; i < 40; ++i)
    {
        fillTone (a);
        fillTone (b);
        fillTone (c);
        empty.clear();
        cc1.clear();
        cc1.addEvent (juce::MidiMessage::controllerEvent (1, 1, 127), 0);
        hostOnly.processBlock (a, empty);
        midiOnly.processBlock (b, cc1);
        combined.processBlock (c, cc1);

        for (int s = 0; s < 256; ++s)
        {
            energyHost += std::abs (a.getSample (0, s));
            energyMidi += std::abs (b.getSample (0, s));
            energyCombined += std::abs (c.getSample (0, s));
        }
    }

    // Combined with host 0.3 + MIDI 1.0 should be near midi-only (max), not stuck at 0.3.
    REQUIRE (energyCombined > energyHost * 0.9);
    REQUIRE (energyCombined > energyMidi * 0.5);
}

TEST_CASE ("v1 MIDI CC1 remains finite across block sizes",
           "[v1][contract][midi][MIDI-10]")
{
    for (int blockSize : { 64, 512, 2048 })
    {
        sendbloom::PluginProcessor plugin;
        // Prepare at 512 so 2048 is oversized (Phase 21 path) when blockSize==2048.
        configureConnectedRest (plugin, blockSize == 2048 ? 512 : blockSize);

        juce::AudioBuffer<float> buffer (2, blockSize);
        juce::MidiBuffer midi;
        fillTone (buffer);
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 100), blockSize / 4);
        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, 0), (3 * blockSize) / 4);

        REQUIRE_NOTHROW (plugin.processBlock (buffer, midi));
        REQUIRE (isFiniteBuffer (buffer));
        REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::sendAmount)
                 == Catch::Approx (0.0f).margin (1.0e-6f));
    }
}
