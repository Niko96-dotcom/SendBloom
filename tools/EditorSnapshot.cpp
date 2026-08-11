#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

#include <juce_graphics/juce_graphics.h>
#include <juce_gui_basics/juce_gui_basics.h>

#include <cmath>

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
    bool bypassed = false;
    bool rotaryMin = false;
    bool rotaryCentre = false;
    bool rotaryMax = false;
    bool presetMenu = false;
    bool presetLongest = false;
    bool presetCustom = false;
    auto presetActionState = sendbloom::PluginEditor::PresetActionSnapshotState::none;
    float renderScale = 1.0f;

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
        else if (arg == "--bypass")
            bypassed = true;
        else if (arg == "--rotary-min")
            rotaryMin = true;
        else if (arg == "--rotary-centre")
            rotaryCentre = true;
        else if (arg == "--rotary-max")
            rotaryMax = true;
        else if (arg == "--preset-menu")
            presetMenu = presetLongest = true;
        else if (arg == "--preset-longest")
            presetLongest = true;
        else if (arg == "--preset-custom")
            presetCustom = true;
        else if (arg == "--load-hover")
            presetActionState = sendbloom::PluginEditor::PresetActionSnapshotState::loadHover;
        else if (arg == "--load-focus")
            presetActionState = sendbloom::PluginEditor::PresetActionSnapshotState::loadFocus;
        else if (arg == "--save-hover")
            presetActionState = sendbloom::PluginEditor::PresetActionSnapshotState::saveHover;
        else if (arg == "--save-focus")
            presetActionState = sendbloom::PluginEditor::PresetActionSnapshotState::saveFocus;
        else if (arg == "--scale")
        {
            if (i + 1 >= argc)
                return 2;

            const auto requestedScale = juce::String { argv[++i] }.getDoubleValue();
            if (! std::isfinite (requestedScale) || requestedScale < 1.0 || requestedScale > 4.0)
                return 2;

            renderScale = static_cast<float> (requestedScale);
        }
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
    if (bypassed)
        setParam (processor, bypass, 1.0f);

    if (presetLongest)
        processor.setCurrentProgram (2); // CUT SAMPLE GATE: longest factory display name.

    if (rotaryMin || rotaryCentre || rotaryMax)
    {
        const auto value = rotaryMin ? 0.0f : (rotaryMax ? 1.0f : 0.5f);
        for (const auto* id : { inputGain, size, level, distn, outputGain })
            setParam (processor, id, value);
    }

    if (presetCustom)
    {
        processor.setCurrentProgram (2);
        setParam (processor, size, 0.731f);
    }

    sendbloom::PluginEditor editor (processor);
    editor.setVisible (true);
    editor.resized();

    if (openAdvanced)
        editor.setAdvancedExpandedForSnapshot (true);
    editor.setPresetActionStateForSnapshot (presetActionState);

    // Allow attachments, component visibility, and image-backed child paints to settle
    // before capturing. Immediate construction-frame snapshots can omit child layers.
    juce::MessageManager::getInstance()->runDispatchLoopUntil (30);

    const auto imageWidth = juce::roundToInt (static_cast<float> (editor.getWidth()) * renderScale);
    const auto imageHeight = juce::roundToInt (static_cast<float> (editor.getHeight()) * renderScale);
    juce::Image image (juce::Image::ARGB, imageWidth, imageHeight, true);
    juce::Graphics g (image);
    g.addTransform (juce::AffineTransform::scale (renderScale));
    editor.paintEntireComponent (g, true);
    if (presetMenu)
        editor.paintPresetMenuForSnapshot (g);

    juce::PNGImageFormat format;
    output.deleteFile(); // FileOutputStream appends; a stale first PNG stream would mask every new snapshot
    juce::FileOutputStream stream (output);
    if (! stream.openedOk() || ! format.writeImageToStream (image, stream))
        return 1;

    return 0;
}
