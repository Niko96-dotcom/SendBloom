#include "PluginProcessor.h"

#include <juce_gui_basics/juce_gui_basics.h>

namespace
{
class PreviewWindow final : public juce::DocumentWindow
{
public:
    explicit PreviewWindow (sendbloom::PluginProcessor& processor)
        : DocumentWindow ("SendBloom local preview (no audio device)", juce::Colours::black,
                          DocumentWindow::closeButton)
    {
        setUsingNativeTitleBar (true);
        setContentOwned (processor.createEditor(), true);
        centreWithSize (getWidth(), getHeight());
        setVisible (true);
    }

    void closeButtonPressed() override
    {
        juce::JUCEApplication::getInstance()->systemRequestedQuit();
    }
};

class PreviewApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "SendBloom Editor Preview"; }
    const juce::String getApplicationVersion() override { return VERSION; }

    void initialise (const juce::String&) override
    {
        // No audio device, persistent settings or installed plug-in are needed.
        // This host is for disposable preset/control/state interaction only.
        processor = std::make_unique<sendbloom::PluginProcessor>();
        window = std::make_unique<PreviewWindow> (*processor);
    }

    void shutdown() override
    {
        window.reset();
        processor.reset();
    }

private:
    std::unique_ptr<sendbloom::PluginProcessor> processor;
    std::unique_ptr<PreviewWindow> window;
};
}

START_JUCE_APPLICATION (PreviewApplication)
