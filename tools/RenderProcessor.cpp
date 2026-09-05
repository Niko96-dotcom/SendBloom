#include "PluginProcessor.h"
#include <cmath>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <vector>

// Local fixture tool: mono native float32 input/output, JSON physical parameter
// values and sample-positioned CC1 events. Runs the actual shipping processor,
// including input stage, smoothing, gate, SRC, wet dirt, dry mix and PDC.
int main (int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc != 4)
            throw std::runtime_error ("usage: RenderProcessor input.f32 output.f32 settings.json");
        const auto config = juce::JSON::parse (juce::File (juce::String (argv[3])));
        if (! config.isObject())
            throw std::runtime_error ("invalid settings JSON");
        const double rate = config["rate"];
        const int block = config["block"];
        if (! std::isfinite (rate) || rate < 8000 || rate > 192000 || block < 1 || block > 8192)
            throw std::runtime_error ("invalid rate/block");
        std::ifstream input (argv[1], std::ios::binary | std::ios::ate);
        const auto bytes = input.tellg();
        if (! input || bytes <= 0 || bytes % 4 != 0 || bytes > 192000 * 4 * 120)
            throw std::runtime_error ("invalid or oversized input (maximum 120s at 192k)");
        std::vector<float> samples (static_cast<size_t> (bytes) / sizeof (float));
        input.seekg (0);
        input.read (reinterpret_cast<char*> (samples.data()), bytes);
        if (! input)
            throw std::runtime_error ("input read failed");
        for (auto x : samples)
            if (! std::isfinite (x))
                throw std::runtime_error ("nonfinite input");
        const juce::File outputFile { juce::String (argv[2]) };
        if (outputFile.exists())
            throw std::runtime_error ("output must be new");

        sendbloom::PluginProcessor plugin;
        const auto parameters = config["parameters"];
        if (auto* object = parameters.getDynamicObject())
            for (const auto& entry : object->getProperties())
            {
                auto* parameter = plugin.getAPVTS().getParameter (entry.name.toString());
                const float value = entry.value;
                if (parameter == nullptr || ! std::isfinite (value)
                    || value < parameter->getNormalisableRange().start
                    || value > parameter->getNormalisableRange().end)
                    throw std::runtime_error ("unknown or out-of-range parameter");
                parameter->setValueNotifyingHost (parameter->convertTo0to1 (value));
            }
        plugin.prepareToPlay (rate, block);
        juce::AudioBuffer<float> buffer (2, block);
        juce::MidiBuffer midi;
        for (size_t offset = 0; offset < samples.size(); offset += static_cast<size_t> (block))
        {
            const int count = static_cast<int> (std::min (static_cast<size_t> (block), samples.size() - offset));
            buffer.setSize (2, count, false, false, true);
            for (int ch = 0; ch < 2; ++ch)
                buffer.copyFrom (ch, 0, samples.data() + offset, count);
            midi.clear();
            if (auto* events = config["cc1"].getArray())
                for (const auto& event : *events)
                {
                    const int position = event["sample"];
                    const int value = event["value"];
                    if (position < 0 || value < 0 || value > 127)
                        throw std::runtime_error ("invalid CC1 event");
                    if (static_cast<size_t> (position) >= offset
                        && static_cast<size_t> (position) < offset + static_cast<size_t> (count))
                        midi.addEvent (juce::MidiMessage::controllerEvent (1, 1, value),
                                       position - static_cast<int> (offset));
                }
            plugin.processBlock (buffer, midi);
            std::copy_n (buffer.getReadPointer (0), count, samples.data() + offset);
        }
        for (auto x : samples)
            if (! std::isfinite (x))
                throw std::runtime_error ("nonfinite output");
        std::ofstream output (argv[2], std::ios::binary | std::ios::trunc);
        output.write (reinterpret_cast<const char*> (samples.data()),
                      static_cast<std::streamsize> (samples.size() * sizeof (float)));
        output.close();
        if (! output)
            throw std::runtime_error ("output write failed");
        std::cout << "{\"latency_samples\":" << plugin.getLatencySamples()
                  << ",\"samples\":" << samples.size() << ",\"rate\":" << rate << "}\n";
        plugin.releaseResources();
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
