#include "PluginEditor.h"
#include "ParameterCurves.h"
#include <ClearInstrumentsData.h>


#include <cmath>

namespace sendbloom
{

namespace
{

constexpr int kEditorWidth = 840;
constexpr int kEditorHeight = 700;

juce::String upperPresetName (PluginProcessor& processor, int index)
{
    // Editor furniture stays compact and musical.  The host-only API carries
    // the Factory: qualifier needed to distinguish user/project programs.
    return processor.getProgramDisplayName (index).toUpperCase();
}

juce::String formatInputGainDb (double norm)
{
    // CORE-02 / ADR-V1-08: display must call the canonical DSP curve.
    const auto db = ParameterCurves::inputGainDb (static_cast<float> (norm));
    if (std::abs (db) < 0.005f)
        return "0.00";

    return juce::String (db, 2);
}

} // namespace

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p),
      processorRef (p),
      pressurePad (p.getAPVTS(), ParameterIDs::sendConnected, ParameterIDs::sendAmount),
      advancedDrawer (p.getAPVTS(),
                      ParameterIDs::inputThreshold,
                      ParameterIDs::sendFeel,
                      ParameterIDs::extendedStereo,
                      ParameterIDs::sendConnected)
{
    lookAndFeel.setKnobFilmstrip (ui::SendBloomSceneArt::image ("aux-knob-strip.png"), 65);
    lookAndFeel.setDisplayImage (ui::SendBloomSceneArt::image ("aux-display.png"), { 32, 32, 32, 32 }, 5.0f, { 3, 6, 3, 6 });
    lookAndFeel.setButtonImages (ui::SendBloomSceneArt::image ("aux-button-0.png"), ui::SendBloomSceneArt::image ("aux-button-1.png"),
                                { 32, 32, 32, 32 }, 5.0f, { 5, 8, 5, 8 });
    lookAndFeel.setSelectorImage (ui::SendBloomSceneArt::image ("aux-button-0.png"), { 32, 32, 32, 32 }, 5.0f, { 5, 8, 5, 28 });
    lookAndFeel.setSwitchImages (ui::SendBloomSceneArt::image ("aux-switch-0.png"), ui::SendBloomSceneArt::image ("aux-switch-1.png"));
    setLookAndFeel (&lookAndFeel);
    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
        knob->setLookAndFeel (&sceneLookAndFeel);
    for (auto* control : std::initializer_list<juce::Component*> { &presetBox, &darkToggle, &gateToggle, &bypassToggle, &loadPresetButton, &savePresetButton, &advancedButton })
        control->setLookAndFeel (&sceneLookAndFeel);
    pressurePad.setSceneMode (true);

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
        presetBox.addItem (upperPresetName (processorRef, i), i + 1);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.setText (processorRef.getCurrentProgramDisplayName().toUpperCase(), juce::dontSendNotification);
    presetBox.setName ("Preset");
    presetBox.setTitle ("Preset");
    presetBox.setDescription ("SendBloom factory and custom preset selector");
    presetBox.setHelpText ("Choose a factory preset. Edited parameter values are shown as Custom.");
    presetBox.onChange = [this]
    {
        presetChanged();
        repaint();
    };
    addAndMakeVisible (presetBox);

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
        addAndMakeVisible (*knob);

    lvlKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 0) + " %"; });
    sizeKnob.setComponentID (ParameterIDs::size);
    sizeKnob.setValueFormatter ([] (double value)
    {
        const auto rt60 = ParameterCurves::sizeToRT60 (static_cast<float> (value));
        return juce::String (rt60, 2) + " s";
    });
    distnKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 0) + " %"; });
    inKnob.setValueFormatter ([] (double value) { return formatInputGainDb (value) + " dB"; });
    outKnob.setValueFormatter ([] (double value) { return juce::String (value, 1) + " dB"; });

    // Match established pro-audio interaction: every rotary has an explicit
    // hardware default and can be reset without hunting through a preset.
    inKnob.setDefaultValue (0.5);
    sizeKnob.setDefaultValue (0.5);
    lvlKnob.setDefaultValue (0.5);
    distnKnob.setDefaultValue (0.0);
    outKnob.setDefaultValue (0.0);

    darkToggle.setButtonText ("DARK SOUND");
    darkToggle.setTooltip ("Darkens the reverb sound; does not change the interface theme.");
    gateToggle.setTooltip ("Move the gate before or after the reverb.");
    gateToggle.getProperties().set ("niko.switchCompact", true);
    addAndMakeVisible (bypassToggle);
    darkToggle.setClickingTogglesState (true);
    gateToggle.setClickingTogglesState (true);
    addAndMakeVisible (darkToggle);
    addAndMakeVisible (gateToggle);

    pressurePad.setOpaque (false);
    addAndMakeVisible (pressurePad);

    advancedButton.setButtonText ("ADVANCED");
    advancedButton.onClick = [this] { toggleAdvanced(); };
    addAndMakeVisible (advancedButton);

    for (auto* button : { &loadPresetButton, &savePresetButton })
    {
        button->setButtonText (button == &loadPresetButton ? "LOAD" : "SAVE");
        button->setWantsKeyboardFocus (true);
        addAndMakeVisible (*button);
    }
    loadPresetButton.setTooltip ("Load a SendBloom preset file");
    savePresetButton.setTooltip ("Save the current SendBloom state");
    loadPresetButton.setName ("Load preset");
    loadPresetButton.setTitle ("Load preset");
    loadPresetButton.setDescription ("Load a SendBloom preset file");
    loadPresetButton.setHelpText ("Opens a file chooser for a SendBloom preset file.");
    savePresetButton.setName ("Save preset");
    savePresetButton.setTitle ("Save preset");
    savePresetButton.setDescription ("Save the current SendBloom state");
    savePresetButton.setHelpText ("Opens a file chooser to save the current SendBloom state.");
    loadPresetButton.onClick = [this] { loadPresetFromDisk(); };
    savePresetButton.onClick = [this] { savePresetToDisk(); };
    addChildComponent (advancedDrawer);

    auto& apvts = processorRef.getAPVTS();

    inAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::inputGain, inKnob.getSlider());
    sizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::size, sizeKnob.getSlider());
    lvlAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::level, lvlKnob.getSlider());
    distnAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::distn, distnKnob.getSlider());
    outAttachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (
        apvts, ParameterIDs::outputGain, outKnob.getSlider());
    darkAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::darkMode, darkToggle);
    gateAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::gatePrePost, gateToggle);
    bypassAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (
        apvts, ParameterIDs::bypass, bypassToggle);
    gateToggle.setButtonText (gateToggle.getToggleState() ? "GATE POST" : "GATE PRE");
    gateToggle.onStateChange = [this]
    {
        gateToggle.setButtonText (gateToggle.getToggleState() ? "GATE POST" : "GATE PRE");
        repaint();
    };

    darkToggle.onStateChange = [this] { repaint(); };

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob }) knob->setLookAndFeel (nullptr);
    bypassToggle.setLookAndFeel (nullptr);
    advancedButton.setLookAndFeel (nullptr);
    presetBox.setLookAndFeel (nullptr);
    darkToggle.setLookAndFeel (nullptr);
    gateToggle.setLookAndFeel (nullptr);
    loadPresetButton.setLookAndFeel (nullptr);
    savePresetButton.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    if (! sceneArt.valid)
    {
        g.fillAll (juce::Colour(0xffe4e5e0));
        g.setColour (niko::clear::palette::ink);
        g.drawText ("SendBloom scene assets are incomplete", getLocalBounds(), juce::Justification::centred);
        return;
    }
    g.drawImage (sceneArt.base, getLocalBounds().toFloat());
    auto& apvts = processorRef.getAPVTS();
    for (const auto& item : sceneArt.knobs)
        if (const auto* parameter = apvts.getParameter(item.first))
            item.second.paint (g, juce::roundToInt(parameter->getValue() * 64.0f));
    const auto liveAmount = apvts.getParameter(ParameterIDs::sendAmount)->getValue();
    auto travel = pressurePad.getPressTravel();
    if (travel < 0.0f)
        travel = pressurePad.isPressed() || pressurePad.getDisplayAmount() > 0.001f || liveAmount > 0.001f ? 1.0f : 0.0f;
    sceneArt.pressure.paint (g, juce::roundToInt(travel * float(sceneArt.pressure.frames.size() - 1)));
    for (const auto& group : sceneArt.discrete)
    {
        int index = 0, bit = 0;
        if (const auto* sources = group.sources.getArray())
            for (const auto& source : *sources)
            {
                const auto kind = source["kind"].toString(), id = source["id"].toString();
                bool state = false;
                if (kind == "parameter")
                {
                    if (const auto* parameter = apvts.getParameter(id)) state = parameter->getValue() > 0.5f;
                }
                else if (kind == "processor_flag" && id == "clip") state = processorRef.isClipHoldActive();
                else if (kind == "button_down")
                    state = id == "load_preset" ? loadPresetButton.isDown() : id == "save_preset" ? savePresetButton.isDown() : id == "advanced" && advancedButton.isDown();
                if (state) index |= (1 << bit);
                ++bit;
            }
        group.paint (g, index);
    }
    if (! advancedDrawer.isExpanded())
    {
        g.setColour (juce::Colour(0xffeee9db));
        g.setFont (niko::clear::numeric(13.0f));
        g.drawText (juce::String(juce::roundToInt(liveAmount * 100.0f)) + " %", sceneArt.display(ParameterIDs::sendAmount), juce::Justification::centred);
    }
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    const auto paintActionFeedback = [&g] (juce::Button& button,
                                           juce::Rectangle<int> bounds,
                                           bool forceHover,
                                           bool forceFocus)
    {
        const auto hovered = forceHover || button.isMouseOverOrDragging();
        const auto focused = forceFocus || button.hasKeyboardFocus (true);
        const auto down = button.isDown();
        if (! hovered && ! focused && ! down)
            return;

        const auto ring = bounds.toFloat().expanded (2.0f);
        if (hovered || down)
        {
            g.setColour (niko::clear::palette::signal.withAlpha (down ? 0.24f : 0.12f));
            g.fillRoundedRectangle (ring, 5.0f);
        }

        g.setColour (juce::Colours::black.withAlpha (0.58f));
        g.drawRoundedRectangle (ring.expanded (1.0f), 6.0f, 1.0f);
        g.setColour (niko::clear::palette::signal.withAlpha (focused ? 0.96f : 0.78f));
        g.drawRoundedRectangle (ring, 5.0f, focused ? 2.0f : 1.2f);
    };

    for (auto* control : std::initializer_list<juce::Component*> { &darkToggle, &gateToggle, &bypassToggle, &advancedButton, &presetBox })
        if (control->isVisible() && control->hasKeyboardFocus(true))
        {
            g.setColour(niko::clear::palette::signal);
            g.drawRoundedRectangle(control->getBounds().toFloat().reduced(1.0f), 5.0f, 1.4f);
        }
    using Snapshot = PresetActionSnapshotState;
    paintActionFeedback (loadPresetButton,
                         loadPresetButton.getBounds(),
                         presetActionSnapshotState == Snapshot::loadHover,
                         presetActionSnapshotState == Snapshot::loadFocus);
    paintActionFeedback (savePresetButton,
                         savePresetButton.getBounds(),
                         presetActionSnapshotState == Snapshot::saveHover,
                         presetActionSnapshotState == Snapshot::saveFocus);
}

void PluginEditor::resized()
{
    presetBox.setBounds (sceneArt.hit("preset"));
    const auto presetText = sceneArt.display("ui_preset").translated(-float(presetBox.getX()), -float(presetBox.getY()));
    presetBox.getProperties().set("scene.text.x", presetText.getX());
    presetBox.getProperties().set("scene.text.y", presetText.getY());
    presetBox.getProperties().set("scene.text.width", presetText.getWidth());
    presetBox.getProperties().set("scene.text.height", presetText.getHeight());
    presetBox.resized();
    loadPresetButton.setBounds (sceneArt.hit("load_preset"));
    savePresetButton.setBounds (sceneArt.hit("save_preset"));
    const auto placeKnob = [this] (ui::PedalKnob& knob, const char* id)
    {
        const auto hit = sceneArt.hit(id);
        const auto value = sceneArt.display(id);
        const auto bounds = hit.getUnion(value.getSmallestIntegerContainer());
        knob.setBounds(bounds);
        knob.setSceneGeometry(hit.translated(-bounds.getX(), -bounds.getY()), value.translated(-float(bounds.getX()), -float(bounds.getY())));
    };
    placeKnob(inKnob, ParameterIDs::inputGain);
    placeKnob(distnKnob, ParameterIDs::distn);
    placeKnob(sizeKnob, ParameterIDs::size);
    placeKnob(lvlKnob, ParameterIDs::level);
    placeKnob(outKnob, ParameterIDs::outputGain);
    darkToggle.setBounds (sceneArt.hit(ParameterIDs::darkMode));
    gateToggle.setBounds (sceneArt.hit(ParameterIDs::gatePrePost));
    pressurePad.setBounds (sceneArt.hit(ParameterIDs::sendAmount));
    bypassToggle.setBounds (sceneArt.hit(ParameterIDs::bypass));
    advancedDrawer.setBounds (getAdvancedBounds());
    advancedButton.setBounds (sceneArt.hit("advanced"));
}

void PluginEditor::presetChanged()
{
    const auto index = presetBox.getSelectedId() - 1;
    if (index >= 0)
        processorRef.setCurrentProgram (index);
}

void PluginEditor::loadPresetFromDisk()
{
    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Load SendBloom preset", juce::File(), "*.sendbloom");
    juce::Component::SafePointer<PluginEditor> safeThis (this);
    presetFileChooser->launchAsync (juce::FileBrowserComponent::openMode
                                      | juce::FileBrowserComponent::canSelectFiles,
                                    [safeThis] (const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        const auto file = chooser.getResult();
        juce::MemoryBlock state;
        if (file.existsAsFile() && file.loadFileAsData (state))
        {
            safeThis->processorRef.setStateInformation (state.getData(), static_cast<int> (state.getSize()));
            safeThis->repaint();
        }
        safeThis->presetFileChooser.reset();
    });
}

void PluginEditor::savePresetToDisk()
{
    presetFileChooser = std::make_unique<juce::FileChooser> (
        "Save SendBloom preset", juce::File::getSpecialLocation (juce::File::userDocumentsDirectory)
                                     .getChildFile ("SendBloom.sendbloom"),
        "*.sendbloom");
    juce::Component::SafePointer<PluginEditor> safeThis (this);
    presetFileChooser->launchAsync (juce::FileBrowserComponent::saveMode
                                      | juce::FileBrowserComponent::canSelectFiles
                                      | juce::FileBrowserComponent::warnAboutOverwriting,
                                    [safeThis] (const juce::FileChooser& chooser)
    {
        if (safeThis == nullptr)
            return;

        auto file = chooser.getResult();
        if (file != juce::File())
        {
            if (! file.hasFileExtension ("sendbloom"))
                file = file.withFileExtension ("sendbloom");

            juce::MemoryBlock state;
            safeThis->processorRef.getStateInformation (state);
            file.replaceWithData (state.getData(), state.getSize());
        }
        safeThis->presetFileChooser.reset();
    });
}

void PluginEditor::toggleAdvanced()
{
    setAdvancedExpandedForSnapshot (! advancedDrawer.isExpanded());
}

juce::Rectangle<int> PluginEditor::getAdvancedBounds() const
{
    return { 65, 467, 544, 152 };
}

void PluginEditor::setAdvancedExpandedForSnapshot (bool shouldExpand)
{
    // Covered controls must leave keyboard traversal while the settings drawer is open.
    darkToggle.setVisible (! shouldExpand);
    gateToggle.setVisible (! shouldExpand);
    pressurePad.setVisible (! shouldExpand);
    bypassToggle.setVisible (true);
    advancedDrawer.setExpanded (shouldExpand);
    advancedButton.setButtonText (shouldExpand ? "CLOSE" : "ADVANCED");
    resized();
    if (shouldExpand)
    {
        advancedDrawer.toFront (false);
        advancedButton.toFront (false); // keep the close target clickable above the drawer
    }
    repaint();
}

void PluginEditor::setPresetActionStateForSnapshot (PresetActionSnapshotState state)
{
    presetActionSnapshotState = state;
    loadPresetButton.setState (state == PresetActionSnapshotState::loadDown ? juce::Button::buttonDown : juce::Button::buttonNormal);
    savePresetButton.setState (state == PresetActionSnapshotState::saveDown ? juce::Button::buttonDown : juce::Button::buttonNormal);
    advancedButton.setState (state == PresetActionSnapshotState::advancedDown ? juce::Button::buttonDown : juce::Button::buttonNormal);
    repaint();
}

void PluginEditor::paintPresetMenuForSnapshot (juce::Graphics& g)
{
    // PopupMenu uses a separate peer in a host. The snapshot executable paints
    // the same LookAndFeel calls into the deterministic editor image so the menu
    // palette, typography, selection, and longest factory name are reviewable.
    constexpr int rowHeight = 27;
    constexpr int inset = 4;
    const auto menuBounds = juce::Rectangle<int> (presetBox.getX(), presetBox.getBottom() + 6, 270,
                                                   inset * 2 + processorRef.getNumPrograms() * rowHeight);

    g.setColour (juce::Colours::black.withAlpha (0.24f));
    g.fillRoundedRectangle (menuBounds.toFloat().translated (3.0f, 4.0f), 4.0f);

    juce::Graphics::ScopedSaveState saved (g);
    g.reduceClipRegion (menuBounds);
    g.setOrigin (menuBounds.getPosition());
    lookAndFeel.drawPopupMenuBackground (g, menuBounds.getWidth(), menuBounds.getHeight());

    for (int i = 0; i < processorRef.getNumPrograms(); ++i)
    {
        const auto row = juce::Rectangle<int> (inset,
                                                inset + i * rowHeight,
                                                menuBounds.getWidth() - inset * 2,
                                                rowHeight);
        lookAndFeel.drawPopupMenuItem (g,
                                               row,
                                               false,
                                               true,
                                               i == processorRef.getCurrentProgram(),
                                               i == processorRef.getCurrentProgram(),
                                               false,
                                               upperPresetName (processorRef, i),
                                               {},
                                               nullptr,
                                               nullptr);
    }

    g.setColour (juce::Colour (0xff16191b).withAlpha (0.82f));
    g.drawRoundedRectangle (juce::Rectangle<float> (0.5f, 0.5f,
                                                     static_cast<float> (menuBounds.getWidth() - 1),
                                                     static_cast<float> (menuBounds.getHeight() - 1)),
                            4.0f,
                            1.0f);
}

void PluginEditor::timerCallback()
{
    const auto programId = processorRef.getCurrentProgram() + 1;
    if (presetBox.getSelectedId() != programId)
        presetBox.setSelectedId (programId, juce::dontSendNotification);

    const auto displayName = processorRef.getCurrentProgramDisplayName().toUpperCase();
    if (presetBox.getText() != displayName)
        presetBox.setText (displayName, juce::dontSendNotification);
    repaint();
}

} // namespace sendbloom
