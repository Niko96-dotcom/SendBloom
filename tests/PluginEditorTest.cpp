#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <ParameterIDs.h>
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

TEST_CASE ("Faceplate control hotspots are hittable and paint knobs", "[ui][editor][interactive]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);
    editor.setVisible (true);
    editor.resized();

    const juce::Point<int> levelCentre { 265, 213 };
    const juce::Point<int> darkCentre { 89, 593 };
    const juce::Point<int> advancedCentre { 316, 698 };

    auto* levelHit = editor.getComponentAt (levelCentre);
    auto* darkHit = editor.getComponentAt (darkCentre);
    auto* advancedHit = editor.getComponentAt (advancedCentre);

    REQUIRE (levelHit != nullptr);
    REQUIRE (levelHit != &editor);
    REQUIRE (darkHit != nullptr);
    REQUIRE (darkHit != &editor);
    REQUIRE (advancedHit != nullptr);
    REQUIRE (advancedHit != &editor);

    auto& apvts = processor.getAPVTS();
    auto* level = apvts.getParameter (sendbloom::ParameterIDs::level);
    REQUIRE (level != nullptr);

    const auto before = level->getValue();
    level->setValueNotifyingHost (before < 0.5f ? 0.9f : 0.1f);
    REQUIRE (std::abs (level->getValue() - before) > 0.3f);

    juce::Image image (juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics g (image);
    editor.paintEntireComponent (g, true);

    // Cropped knob art is dark; punched faceplate hole is light. Centre should be dark.
    const auto centre = image.getPixelAt (levelCentre.x, levelCentre.y);
    REQUIRE (centre.getBrightness() < 0.35f);
}

TEST_CASE ("Clip hold flag accessible from processor", "[ui][clip]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE_FALSE (plugin.isClipHoldActive());
}
