#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <ParameterIDs.h>
#include <ui/PedalFaceplatePaint.h>
#include <catch2/catch_test_macros.hpp>
#include <juce_gui_basics/juce_gui_basics.h>

namespace
{

bool containsButtonText (juce::Component& component, const juce::String& text)
{
    if (auto* button = dynamic_cast<juce::Button*> (&component))
        if (button->getButtonText() == text)
            return true;

    for (auto* child : component.getChildren())
        if (containsButtonText (*child, text))
            return true;

    return false;
}

} // namespace

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

    // Probe the shared faceplate layout rectangles the editor parks its hotspots on.
    using namespace sendbloom::ui::facelayout;
    const auto levelCentre = kLevelKnob.withHeight (kKnobLarge).getCentre();
    const auto darkCentre = kDarkButton.getCentre();
    const auto advancedCentre = kAdvancedHitBox.getCentre();

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
}

TEST_CASE ("Faceplate knob rendering keeps its moulded body and bright index",
           "[ui][editor][render]")
{
#if ! JUCE_MAC
    SKIP ("Rendered-pixel brightness is a macOS-referenced contract. The faceplate art is "
          "path-traced and reviewed on macOS, and macOS AU/VST3 are the only released "
          "artifacts, so another platform's rasteriser disagreeing here measures the "
          "rasteriser rather than the art.");
#else
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);
    editor.setVisible (true);
    editor.resized();

    using namespace sendbloom::ui::facelayout;
    const auto levelCentre = kLevelKnob.withHeight (kKnobLarge).getCentre();

    juce::Image image (juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics g (image);
    editor.paintEntireComponent (g, true);

    // Dark control over the bright clear-shell register: prove the rendered
    // hardware still contains its moulded body and physical index/washer.
    float darkest = 1.0f;
    float brightest = 0.0f;
    for (int y = levelCentre.y - 30; y <= levelCentre.y + 30; ++y)
        for (int x = levelCentre.x - 30; x <= levelCentre.x + 30; ++x)
        {
            const auto brightness = image.getPixelAt (x, y).getBrightness();
            darkest = juce::jmin (darkest, brightness);
            brightest = juce::jmax (brightest, brightness);
        }
    REQUIRE (darkest < 0.22f);
    REQUIRE (brightest > 0.48f);
    REQUIRE (brightest - darkest > 0.35f);
#endif
}

TEST_CASE ("Bright clear-shell board remains neutral and depth-separated at 1x",
           "[ui][editor][render][clearshell]")
{
#if ! JUCE_MAC
    SKIP ("Rendered-pixel colour is a macOS-referenced contract.");
#else
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);
    editor.setVisible (true);
    editor.resized();
    juce::MessageManager::getInstance()->runDispatchLoopUntil (30);

    juce::Image image (juce::Image::ARGB, editor.getWidth(), editor.getHeight(), true);
    juce::Graphics g (image);
    editor.paintEntireComponent (g, true);

    // This region includes exposed white-soldermask board, traces, internal
    // parts and the dark user-contact hardware.  Its mean must stay bright and
    // neutral, while a material fraction of dark pixels prevents a featureless
    // white card from satisfying the ClearShell contract.
    const juce::Rectangle<int> boardRegion { 55, 370, 310, 320 };
    double red = 0.0;
    double green = 0.0;
    double blue = 0.0;
    int darkPixels = 0;
    int brightPixels = 0;
    int sampleCount = 0;

    for (int y = boardRegion.getY(); y < boardRegion.getBottom(); ++y)
        for (int x = boardRegion.getX(); x < boardRegion.getRight(); ++x)
        {
            const auto colour = image.getPixelAt (x, y);
            red += colour.getFloatRed();
            green += colour.getFloatGreen();
            blue += colour.getFloatBlue();
            const auto brightness = colour.getBrightness();
            darkPixels += brightness < 0.35f ? 1 : 0;
            brightPixels += brightness > 0.60f ? 1 : 0;
            ++sampleCount;
        }

    const auto meanRed = red / sampleCount;
    const auto meanGreen = green / sampleCount;
    const auto meanBlue = blue / sampleCount;
    const auto meanMax = juce::jmax (meanRed, juce::jmax (meanGreen, meanBlue));
    const auto meanMin = juce::jmin (meanRed, juce::jmin (meanGreen, meanBlue));
    const auto darkFraction = static_cast<double> (darkPixels) / sampleCount;
    const auto brightFraction = static_cast<double> (brightPixels) / sampleCount;

    REQUIRE (meanMin > 0.42);
    REQUIRE (meanMax - meanMin < 0.06);
    REQUIRE (brightFraction > 0.50);
    REQUIRE (darkFraction > 0.06);
#endif
}

TEST_CASE ("Clip hold flag accessible from processor", "[ui][clip]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE_FALSE (plugin.isClipHoldActive());
}

TEST_CASE ("Advanced drawer omits 32k Color and keeps Extended Stereo", "[ui][editor][advanced]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);
    editor.setAdvancedExpandedForSnapshot (true);

    REQUIRE_FALSE (containsButtonText (editor, "32k Color"));
    REQUIRE (containsButtonText (editor, "Extended Stereo"));
}

TEST_CASE ("Gate control follows preset changes and has no inert preset action buttons",
           "[ui][editor][preset][regression]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);

    juce::ToggleButton* gateButton = nullptr;

    for (auto* child : editor.getChildren())
    {
        if (auto* button = dynamic_cast<juce::TextButton*> (child))
        {
            REQUIRE (button->getButtonText() != "SAVE");
            REQUIRE (button->getButtonText() != "NEW");
            REQUIRE (button->getButtonText() != "DELETE");
        }

        if (auto* toggle = dynamic_cast<juce::ToggleButton*> (child))
            if (toggle->getButtonText() == "Gate")
                gateButton = toggle;
    }

    REQUIRE (gateButton != nullptr);

    processor.setCurrentProgram (1); // Sparkle Verb: gate Post
    juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    REQUIRE (gateButton->getToggleState());

    processor.setCurrentProgram (4); // Dry Dub Sends: gate Pre
    juce::MessageManager::getInstance()->runDispatchLoopUntil (20);
    REQUIRE_FALSE (gateButton->getToggleState());
}

TEST_CASE ("Editor program selector follows project state restore",
           "[ui][editor][preset][state][regression]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    sendbloom::PluginEditor editor (processor);

    juce::ComboBox* presetBox = nullptr;
    for (auto* child : editor.getChildren())
        if (auto* combo = dynamic_cast<juce::ComboBox*> (child))
            presetBox = combo;

    REQUIRE (presetBox != nullptr);
    REQUIRE (presetBox->getSelectedId() == 1); // Init
    // ComboBox item access is zero-based, unlike the one-based item IDs used
    // for host program selection below.
    REQUIRE (presetBox->getItemText (0) == "INIT");
    REQUIRE (presetBox->getItemText (7) == "GATED ROOM");

    sendbloom::PluginProcessor source;
    source.setCurrentProgram (7); // Gated Room
    juce::MemoryBlock state;
    source.getStateInformation (state);
    processor.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
    juce::MessageManager::getInstance()->runDispatchLoopUntil (80);

    REQUIRE (processor.getCurrentProgramDisplayName() == "Gated Room");
    REQUIRE (presetBox->getSelectedId() == 8);
}
