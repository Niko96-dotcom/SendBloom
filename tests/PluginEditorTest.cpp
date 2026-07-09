#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

TEST_CASE ("PluginEditor instantiates at pedal dimensions", "[ui][editor]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);

    REQUIRE (editor.getWidth() == 420);
    REQUIRE (editor.getHeight() == 780);
    REQUIRE (editor.getNumChildComponents() > 5);
}

TEST_CASE ("Clip hold flag accessible from processor", "[ui][clip]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE_FALSE (plugin.isClipHoldActive());
}
