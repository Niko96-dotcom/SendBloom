#include <IReverbEngine.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <PressureSend.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <ui/PressureSendPad.h>
#include <atomic>
#include <cmath>
#include <memory>

namespace
{

constexpr double kSampleRate = 48000.0;
constexpr int kBlockSize = 512;

/** Tracks absolute energy arriving at the reverb input (post PressureSend). */
class EnergyTrackingReverb final : public sendbloom::IReverbEngine
{
public:
    void prepare (double, int) noexcept override {}

    float processSample (float input, float, float, bool) noexcept override
    {
        energy.fetch_add (static_cast<double> (std::abs (input)), std::memory_order_relaxed);
        return input;
    }

    void resetEnergy() noexcept { energy.store (0.0, std::memory_order_relaxed); }
    double getEnergy() const noexcept { return energy.load (std::memory_order_relaxed); }

private:
    std::atomic<double> energy { 0.0 };
};

juce::MouseEvent makeMouseEvent (juce::Component& component, juce::Point<float> pos)
{
    auto source = juce::Desktop::getInstance().getMainMouseSource();
    const auto now = juce::Time::getCurrentTime();
    return juce::MouseEvent (source,
                             pos,
                             juce::ModifierKeys(),
                             1.0f,
                             0.0f,
                             0.0f,
                             0.0f,
                             0.0f,
                             &component,
                             &component,
                             now,
                             pos,
                             now,
                             1,
                             false);
}

void configurePressurePlugin (sendbloom::PluginProcessor& plugin)
{
    using namespace sendbloom::ParameterIDs;

    auto& apvts = plugin.getAPVTS();
    *apvts.getRawParameterValue (inputGain) = 0.5f;
    *apvts.getRawParameterValue (outputGain) = 0.0f;
    *apvts.getRawParameterValue (bypass) = 0.0f;
    *apvts.getRawParameterValue (level) = 1.0f;
    *apvts.getRawParameterValue (distn) = 0.0f;
    *apvts.getRawParameterValue (size) = 0.35f;
    *apvts.getRawParameterValue (gatePrePost) = 0.0f;
    *apvts.getRawParameterValue (sendConnected) = 0.0f;
    *apvts.getRawParameterValue (sendAmount) = 0.0f;
    *apvts.getRawParameterValue (inputThreshold) = 0.0f;
}

void processToneBlocks (sendbloom::PluginProcessor& plugin, int blocks)
{
    juce::AudioBuffer<float> buffer (2, kBlockSize);
    juce::MidiBuffer midi;
    auto phase = 0.0f;

    for (int b = 0; b < blocks; ++b)
    {
        for (int i = 0; i < kBlockSize; ++i)
        {
            const auto sample = 0.35f * std::sin (phase);
            phase += 0.07f;
            buffer.setSample (0, i, sample);
            buffer.setSample (1, i, sample);
        }

        plugin.processBlock (buffer, midi);
    }
}

float bufferRms (const juce::AudioBuffer<float>& buffer)
{
    double sum = 0.0;
    const auto n = buffer.getNumSamples() * buffer.getNumChannels();

    for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            const auto s = buffer.getSample (ch, i);
            sum += static_cast<double> (s) * static_cast<double> (s);
        }

    return static_cast<float> (std::sqrt (sum / static_cast<double> (n)));
}

} // namespace

TEST_CASE ("v1 pressure release stays connected-at-rest with amount 0 and no new wet feed",
           "[v1][contract][pressure-release][SEND-05]")
{
    // Milestone §8.4 / ADR-V1-02 / SEND-05:
    // After UI press→release, send_connected stays true, send_amount→0 (dry feed),
    // and the reverb must accept no new energy while the dry path stays nonzero.
    juce::ScopedJuceInitialiser_GUI gui;

    sendbloom::PluginProcessor plugin;
    configurePressurePlugin (plugin);
    plugin.prepareToPlay (kSampleRate, kBlockSize);

    auto tracker = std::make_unique<EnergyTrackingReverb>();
    auto* energy = tracker.get();
    energy->prepare (kSampleRate, kBlockSize);
    plugin.chain.setReverbEngineForTests (std::move (tracker));

    sendbloom::ui::PressureSendPad pad (plugin.getAPVTS(),
                                        sendbloom::ParameterIDs::sendConnected,
                                        sendbloom::ParameterIDs::sendAmount);
    pad.setBounds (0, 0, 100, 200);

    // Press near the top of the pad → high send_amount, connected=true.
    pad.mouseDown (makeMouseEvent (pad, { 50.0f, 20.0f }));

    {
        using namespace sendbloom::ParameterIDs;
        REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendConnected) > 0.5f);
        REQUIRE (*plugin.getAPVTS().getRawParameterValue (sendAmount) > 0.5f);
    }

    energy->resetEnergy();
    processToneBlocks (plugin, 4);
    const auto energyWhilePressed = energy->getEnergy();
    REQUIRE (energyWhilePressed > 1.0e-3);

    // Release — v1 requires connected-at-rest (connected=true, amount=0).
    pad.mouseUp (makeMouseEvent (pad, { 50.0f, 20.0f }));

    using namespace sendbloom::ParameterIDs;
    const float connectedAfter = plugin.getAPVTS().getRawParameterValue (sendConnected)->load();
    const float amountAfter = plugin.getAPVTS().getRawParameterValue (sendAmount)->load();

    // Intended failures on current tree: mouseUp sets connected=false and does not zero amount.
    REQUIRE (connectedAfter > 0.5f);
    REQUIRE (amountAfter == Catch::Approx (0.0f).margin (1.0e-4f));

    // Connected-at-rest ⇒ PressureSend gain must be 0 (not the disconnected always-on 1.0).
    REQUIRE (sendbloom::PressureSend::computeGain (amountAfter, connectedAfter > 0.5f, true)
             == Catch::Approx (0.0f).margin (1.0e-6f));

    energy->resetEnergy();
    juce::AudioBuffer<float> dryProbe (2, kBlockSize);
    juce::MidiBuffer midi;
    for (int i = 0; i < kBlockSize; ++i)
    {
        dryProbe.setSample (0, i, 0.4f);
        dryProbe.setSample (1, i, 0.4f);
    }
    plugin.processBlock (dryProbe, midi);

    REQUIRE (energy->getEnergy() < 1.0e-6);
    REQUIRE (bufferRms (dryProbe) > 1.0e-3f);
}
