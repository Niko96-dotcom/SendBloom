#include "PluginProcessor.h"
#include "ParameterIDs.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

namespace
{
struct Scenario
{
    const char* name;
    double rate;
    int block;
    int instances;
    int midiStride; // 0: no MIDI; positive: CC1; negative: irrelevant CC2
};

constexpr std::array scenarios {
    Scenario { "audio-48k-64", 48000, 64, 1, 0 },
    Scenario { "audio-48k-512", 48000, 512, 1, 0 },
    Scenario { "audio-96k-128", 96000, 128, 1, 0 },
    Scenario { "audio-192k-128", 192000, 128, 1, 0 },
    Scenario { "audio-8-instances", 48000, 512, 8, 0 },
    Scenario { "pressure-sparse", 48000, 512, 1, 128 },
    Scenario { "pressure-dense", 48000, 512, 1, 16 },
    Scenario { "pressure-stress", 48000, 512, 1, 1 },
    Scenario { "irrelevant-midi", 48000, 512, 1, -1 }
};
constexpr int timedPasses = 20;

struct Workload
{
    explicit Workload (const Scenario& scenario) : s (scenario), buffer (2, s.block)
    {
        const auto blocks = static_cast<int> (std::ceil (s.rate / s.block));
        input.resize (static_cast<size_t> (blocks * s.block));
        midi.resize (static_cast<size_t> (blocks));
        for (int b = 0; b < blocks; ++b)
        {
            for (int i = 0; i < s.block; ++i)
            {
                const auto sample = b * s.block + i;
                const auto t = sample / s.rate;
                // Repeated plucked harmonic signal and silence exercise gate/tail/dirt.
                const auto beat = std::fmod (t, 0.25);
                input[static_cast<size_t> (sample)] = beat < 0.18
                    ? static_cast<float> (0.4 * std::exp (-12.0 * beat)
                        * (std::sin (2.0 * juce::MathConstants<double>::pi * 220.0 * t)
                           + 0.25 * std::sin (2.0 * juce::MathConstants<double>::pi * 660.0 * t)))
                    : 0.0f;
            }
            if (s.midiStride != 0)
                for (int i = 0; i < s.block; i += std::abs (s.midiStride))
                    midi[static_cast<size_t> (b)].addEvent (
                        juce::MidiMessage::controllerEvent (1, s.midiStride > 0 ? 1 : 2,
                                                            (b * 17 + i * 3) % 128), i);
        }
        for (int i = 0; i < s.instances; ++i)
            plugins.push_back (std::make_unique<sendbloom::PluginProcessor>());
    }

    void prepare()
    {
        using namespace sendbloom::ParameterIDs;
        for (auto& p : plugins)
        {
            p->setCurrentProgram (0);
            auto& state = p->getAPVTS();
            *state.getRawParameterValue (inputGain) = 0.6f;
            *state.getRawParameterValue (size) = 0.65f;
            *state.getRawParameterValue (level) = 0.7f;
            *state.getRawParameterValue (distn) = 0.35f;
            *state.getRawParameterValue (darkMode) = 1.0f;
            *state.getRawParameterValue (gatePrePost) = 0.0f;
            *state.getRawParameterValue (sendConnected) = s.midiStride > 0 ? 1.0f : 0.0f;
            p->prepareToPlay (s.rate, s.block);
        }
    }

    // Input and MIDI creation, prepare, file I/O and output validation are untimed.
    // Buffer copies and deterministic parameter changes ARE included in the timing.
    void render (std::vector<float>* output = nullptr)
    {
        for (size_t b = 0; b < midi.size(); ++b)
            for (size_t p = 0; p < plugins.size(); ++p)
            {
                if (b % 32 == 0)
                {
                    auto& state = plugins[p]->getAPVTS();
                    *state.getRawParameterValue (sendbloom::ParameterIDs::size) =
                        b % 64 == 0 ? 0.65f : 0.4f;
                    *state.getRawParameterValue (sendbloom::ParameterIDs::darkMode) =
                        b % 64 == 0 ? 1.0f : 0.0f;
                }
                for (int ch = 0; ch < 2; ++ch)
                    buffer.copyFrom (ch, 0, input.data() + b * static_cast<size_t> (s.block), s.block);
                plugins[p]->processBlock (buffer, midi[b]);
                if (output != nullptr)
                    for (int ch = 0; ch < 2; ++ch)
                        std::copy_n (buffer.getReadPointer (ch), s.block,
                            output->data() + ((b * plugins.size() + p) * 2 + static_cast<size_t> (ch))
                                                * static_cast<size_t> (s.block));
            }
    }

    Scenario s;
    juce::AudioBuffer<float> buffer;
    std::vector<float> input;
    std::vector<juce::MidiBuffer> midi;
    std::vector<std::unique_ptr<sendbloom::PluginProcessor>> plugins;
};
}

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc > 2)
            throw std::runtime_error ("usage: BenchmarkProcessor [new-output-directory]");
        juce::File output;
        if (argc == 2)
        {
            output = juce::File::getCurrentWorkingDirectory().getChildFile (argv[1]);
            if (output.exists() || output.createDirectory().failed())
                throw std::runtime_error ("output directory must be new and writable");
        }
        std::cout << "# SendBloom production processor; build=" << CMAKE_BUILD_TYPE
                  << "; repeats=7; warmup=1s; measured=" << timedPasses
                  << "s; copies/automation included\n"
                  << "scenario,rate,block,instances,min_core_percent,median_core_percent,max_core_percent\n";
        for (const auto& scenario : scenarios)
        {
            Workload w (scenario);
            std::array<double, 7> measurements {};
            for (auto& value : measurements)
            {
                w.prepare();
                w.render();
                const auto start = std::chrono::steady_clock::now();
                for (int pass = 0; pass < timedPasses; ++pass)
                    w.render();
                value = std::chrono::duration<double> (std::chrono::steady_clock::now() - start).count()
                      / (timedPasses * static_cast<double> (w.input.size()) / scenario.rate) * 100.0;
            }
            std::sort (measurements.begin(), measurements.end());
            // Independent, reset render checks EVERY sample and can be byte-compared
            // between baseline/candidate binaries built with the same compiler/options.
            std::vector<float> rendered (w.input.size() * 2 * static_cast<size_t> (scenario.instances));
            w.prepare();
            w.render (&rendered);
            double energy = 0;
            for (auto sample : rendered)
            {
                if (! std::isfinite (sample))
                    throw std::runtime_error ("non-finite output");
                energy += static_cast<double> (sample) * sample;
            }
            if (energy <= 0)
                throw std::runtime_error ("silent output");
            if (argc == 2)
            {
                std::ofstream file (output.getChildFile (juce::String (scenario.name) + ".f32")
                                        .getFullPathName().toStdString(), std::ios::binary);
                file.write (reinterpret_cast<const char*> (rendered.data()),
                            static_cast<std::streamsize> (rendered.size() * sizeof (float)));
                file.close();
                if (! file)
                    throw std::runtime_error ("failed to write output");
            }
            std::cout << scenario.name << ',' << scenario.rate << ',' << scenario.block << ','
                      << scenario.instances << ',' << std::setprecision (7) << measurements.front()
                      << ',' << measurements[3] << ',' << measurements.back() << std::endl;
        }
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
