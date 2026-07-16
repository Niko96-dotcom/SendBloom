#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{

void setParam (sendbloom::PluginProcessor& processor, const char* id, float normalisedValue)
{
    if (auto* param = processor.getAPVTS().getParameter (id))
        param->setValueNotifyingHost (normalisedValue);
}

void triggerClip (sendbloom::PluginProcessor& processor)
{
    using namespace sendbloom::ParameterIDs;

    if (auto* input = processor.getAPVTS().getRawParameterValue (inputGain))
        input->store (1.0f);

    processor.prepareToPlay (48000.0, 64);
    juce::AudioBuffer<float> buffer (2, 64);
    buffer.clear();
    for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
        buffer.setSample (channel, 0, 8.0f);

    juce::MidiBuffer midi;
    processor.processBlock (buffer, midi);
}

} // namespace

int main (int argc, char* argv[])
{
    juce::ScopedJuceInitialiser_GUI gui;

    auto output = juce::File::getCurrentWorkingDirectory().getChildFile ("artifacts/editor-snapshot.png");
    bool openAdvanced = false;
    bool darkOn = false;
    bool gatePre = false;
    bool sendPressed = false;
    bool clipActive = false;

    for (int i = 1; i < argc; ++i)
    {
        const juce::String arg { argv[i] };
        if (arg == "--advanced")
            openAdvanced = true;
        else if (arg == "--dark")
            darkOn = true;
        else if (arg == "--gate-pre")
            gatePre = true;
        else if (arg == "--send")
            sendPressed = true;
        else if (arg == "--clip")
            clipActive = true;
        else
            output = juce::File (arg);
    }

    output.getParentDirectory().createDirectory();
    sendbloom::PluginProcessor processor;

    using namespace sendbloom::ParameterIDs;
    if (darkOn)
        setParam (processor, darkMode, 1.0f);
    if (gatePre)
        setParam (processor, gatePrePost, 0.0f);
    if (sendPressed)
    {
        setParam (processor, sendConnected, 1.0f);
        setParam (processor, sendAmount, 0.75f);
    }
    if (clipActive)
        triggerClip (processor);

    sendbloom::PluginEditor editor (processor);
    editor.setVisible (true);
    editor.resized();

    if (openAdvanced)
        editor.setAdvancedExpandedForSnapshot (true);

    // Allow attachments, component visibility, and image-backed child paints to settle
    // before capturing. Immediate construction-frame snapshots can omit child layers.
    juce::MessageManager::getInstance()->runDispatchLoopUntil (30);

    juce::Image image (juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics g (image);
    editor.paintEntireComponent (g, true);

    juce::PNGImageFormat format;
    output.deleteFile(); // FileOutputStream appends; a stale first PNG stream would mask every new snapshot
    juce::FileOutputStream stream (output);
    if (! stream.openedOk() || ! format.writeImageToStream (image, stream))
        return 1;

    return 0;
}
