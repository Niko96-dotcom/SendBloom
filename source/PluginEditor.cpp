#include "PluginEditor.h"
#include "FactoryPresets.h"
#include "ParameterCurves.h"
#include "ui/PedalFaceplatePaint.h"

#include <cmath>

namespace sendbloom
{

namespace
{

constexpr int kEditorWidth = 420;
constexpr int kEditorHeight = 780;

juce::String upperPresetName (int index)
{
    if (index == 0)
        return "INITIAL PATCH";

    return FactoryPresets::getPresetName (index).toUpperCase();
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
      clipLed ([this] { return processorRef.isClipHoldActive(); }),
      pressurePad (p.getAPVTS(), ParameterIDs::sendConnected, ParameterIDs::sendAmount),
      advancedDrawer (p.getAPVTS(),
                      ParameterIDs::inputThreshold,
                      ParameterIDs::sendFeel,
                      ParameterIDs::authenticColor,
                      ParameterIDs::extendedStereo,
                      ParameterIDs::sendConnected)
{
    setLookAndFeel (&lookAndFeel);

    titleLabel.setVisible (false);
    addAndMakeVisible (titleLabel);

    for (int i = 0; i < FactoryPresets::kNumPresets; ++i)
        presetBox.addItem (upperPresetName (i), i + 1);

    presetBox.setSelectedId (processorRef.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this]
    {
        presetChanged();
        repaint();
    };
    presetBox.setLookAndFeel (&transparentControls);
    presetBox.setColour (juce::ComboBox::backgroundColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::textColourId, juce::Colours::transparentBlack);
    presetBox.setColour (juce::ComboBox::arrowColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (presetBox);
    // Kill the ComboBox's internal label — faceplate owns the default glyphs.
    for (auto* child : presetBox.getChildren())
        if (auto* label = dynamic_cast<juce::Label*> (child))
            label->setAlpha (0.0f);

    for (auto* knob : { &inKnob, &sizeKnob, &lvlKnob, &distnKnob, &outKnob })
        addAndMakeVisible (*knob);

    lvlKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });
    sizeKnob.setValueFormatter ([] (double value) { return juce::String (1.0 + value * 0.2, 2); });
    distnKnob.setValueFormatter ([] (double value) { return juce::String (value * 100.0, 2); });
    inKnob.setValueFormatter ([] (double value) { return formatInputGainDb (value); });
    outKnob.setValueFormatter ([] (double value) { return juce::String (value, 2); });

    darkToggle.setLookAndFeel (&transparentControls);
    gateToggle.setLookAndFeel (&transparentControls);
    darkToggle.setClickingTogglesState (true);
    gateToggle.setClickingTogglesState (true);
    addAndMakeVisible (darkToggle);
    addAndMakeVisible (gateToggle);

    clipLed.setInterceptsMouseClicks (false, false);
    addAndMakeVisible (clipLed);
    clipLed.setVisible (false);

    pressurePad.setOpaque (false);
    addAndMakeVisible (pressurePad);

    advancedButton.setButtonText ("ADVANCED >");
    advancedButton.onClick = [this] { toggleAdvanced(); };
    addAndMakeVisible (advancedButton);

    for (auto* button : { &loadPresetButton, &savePresetButton })
    {
        button->setLookAndFeel (&transparentControls);
        button->setButtonText ({ });
        addAndMakeVisible (*button);
    }
    loadPresetButton.setTooltip ("Load a SendBloom preset file");
    savePresetButton.setTooltip ("Save the current SendBloom state");
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
    gateToggle.setButtonText ("Gate");
    gateToggle.onStateChange = [this] { repaint(); };

    darkToggle.onStateChange = [this] { repaint(); };

    setSize (kEditorWidth, kEditorHeight);
    startTimerHz (30);
}

PluginEditor::~PluginEditor()
{
    presetBox.setLookAndFeel (nullptr);
    darkToggle.setLookAndFeel (nullptr);
    gateToggle.setLookAndFeel (nullptr);
    loadPresetButton.setLookAndFeel (nullptr);
    savePresetButton.setLookAndFeel (nullptr);
    setLookAndFeel (nullptr);
}

void PluginEditor::paint (juce::Graphics& g)
{
    ui::paintPedalFaceplate (g,
                             getLocalBounds().toFloat(),
                             lookAndFeel.cyanColour(),
                             processorRef.getAPVTS(),
                             processorRef.isClipHoldActive(),
                             advancedDrawer.isExpanded(),
                             pressurePad.isPressed(),
                             pressurePad.getDisplayAmount());

    // The photographed face owns the preset field; the editor supplies its live text.
    const auto presetName = presetBox.getSelectedId() != 1 ? presetBox.getText()
                                                           : juce::String ("INITIAL PATCH");
    g.setColour (juce::Colours::black);
    g.setFont (juce::FontOptions (12.0f, juce::Font::bold));
    g.drawText (presetName, ui::facelayout::kPresetText,
                juce::Justification::centredLeft, false);
}

void PluginEditor::paintOverChildren (juce::Graphics& g)
{
    const auto* darkParam = processorRef.getAPVTS().getRawParameterValue (ParameterIDs::darkMode);
    const auto darkOn = darkParam != nullptr && darkParam->load() > 0.5f;

    ui::paintPedalOverlay (g,
                           getLocalBounds().toFloat(),
                           darkOn,
                           processorRef.isClipHoldActive());
}

void PluginEditor::resized()
{
    using namespace ui::facelayout;

    titleLabel.setBounds (0, 0, 0, 0);
    // Invisible hit targets parked on the shared faceplate layout rectangles.
    presetBox.setBounds (kPresetField);
    loadPresetButton.setBounds (kPresetLoad);
    savePresetButton.setBounds (kPresetSave);

    distnKnob.setBounds (kDistortionKnob);
    sizeKnob.setBounds (kSizeKnob);
    lvlKnob.setBounds (kLevelKnob);
    inKnob.setBounds (kInputKnob);
    outKnob.setBounds (kOutputKnob);

    darkToggle.setBounds (kDarkButton);
    gateToggle.setBounds (kGateHitBox);
    pressurePad.setBounds (kFootswitch);

    advancedDrawer.setBounds (getAdvancedBounds());
    // When the drawer is open its title strip becomes the close target.
    advancedButton.setBounds (advancedDrawer.isExpanded()
                                  ? getAdvancedBounds().removeFromTop (36)
                                  : kAdvancedHitBox);
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
    return ui::facelayout::kAdvancedDrawer.withHeight (advancedDrawer.getPreferredHeight());
}

void PluginEditor::setAdvancedExpandedForSnapshot (bool shouldExpand)
{
    advancedDrawer.setExpanded (shouldExpand);
    advancedButton.setButtonText (shouldExpand ? "ADVANCED <" : "ADVANCED >");
    resized();
    if (shouldExpand)
    {
        advancedDrawer.toFront (false);
        advancedButton.toFront (false); // keep the close target clickable above the drawer
    }
    repaint();
}

void PluginEditor::timerCallback()
{
    repaint();
}

} // namespace sendbloom
