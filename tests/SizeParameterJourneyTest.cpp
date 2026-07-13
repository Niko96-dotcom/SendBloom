#include <ParameterCurves.h>
#include <ParameterIDs.h>
#include <ParameterSnapshot.h>
#include <PluginEditor.h>
#include <PluginProcessor.h>
#include <ui/PedalKnob.h>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <array>

namespace
{

sendbloom::ui::PedalKnob* findSizeKnob (sendbloom::PluginEditor& editor)
{
    for (auto* child : editor.getChildren())
        if (auto* knob = dynamic_cast<sendbloom::ui::PedalKnob*> (child))
            if (knob->getComponentID() == sendbloom::ParameterIDs::size)
                return knob;

    return nullptr;
}

struct SizeJourneyCase
{
    const char* label;
    float hostNormalised;
    float expectedRaw;
    float expectedRT60;
    const char* expectedText;
};

} // namespace

TEST_CASE ("Size host automation has one canonical nonlinear mapping",
           "[parm][layout][smooth][ui][regression]")
{
    juce::ScopedJuceInitialiser_GUI gui;
    sendbloom::PluginProcessor processor;
    processor.prepareToPlay (48000.0, 128);
    sendbloom::PluginEditor editor (processor);

    auto* sizeParameter = processor.getAPVTS().getParameter (sendbloom::ParameterIDs::size);
    auto* sizeKnob = findSizeKnob (editor);

    REQUIRE (sizeParameter != nullptr);
    REQUIRE (sizeKnob != nullptr);

    const auto range = sizeParameter->getNormalisableRange();
    const auto defaultHostPosition = sizeParameter->getDefaultValue();

    REQUIRE (defaultHostPosition == Catch::Approx (0.5f));
    REQUIRE (range.convertFrom0to1 (defaultHostPosition) == Catch::Approx (0.5f));
    REQUIRE (range.convertTo0to1 (0.5f) == Catch::Approx (defaultHostPosition));
    REQUIRE (sizeKnob->getSlider().valueToProportionOfLength (0.5)
             == Catch::Approx (defaultHostPosition));

    constexpr auto midpointRT60 = 1.3394213f;
    const std::array<SizeJourneyCase, 4> cases {{
        { "minimum", 0.0f, 0.0f, 0.25f, "0.25 s" },
        { "midpoint", 0.5f, 0.5f, midpointRT60, "1.34 s" },
        { "maximum", 1.0f, 1.0f, 6.0f, "6.00 s" },
        { "default", defaultHostPosition, 0.5f, midpointRT60, "1.34 s" },
    }};

    for (const auto& testCase : cases)
    {
        INFO (testCase.label);

        sizeParameter->setValueNotifyingHost (testCase.hostNormalised);
        juce::MessageManager::getInstance()->runDispatchLoopUntil (20);

        const auto rawValue = processor.getAPVTS()
                                  .getRawParameterValue (sendbloom::ParameterIDs::size)
                                  ->load();
        REQUIRE (rawValue == Catch::Approx (testCase.expectedRaw));

        const auto snapshot = sendbloom::ParameterSnapshot::capture (processor.getAPVTS());
        processor.smoothedBank.setTargets (snapshot);
        processor.smoothedBank.snapToTargets();
        const auto smoothedValue = processor.smoothedBank.getNextSizeNorm();
        REQUIRE (smoothedValue == Catch::Approx (testCase.expectedRaw));

        const auto rt60 = sendbloom::ParameterCurves::sizeToRT60 (smoothedValue);
        REQUIRE (rt60 == Catch::Approx (testCase.expectedRT60));
        REQUIRE (sizeKnob->getSlider().getValue() == Catch::Approx (testCase.expectedRaw));
        REQUIRE (sizeKnob->getSlider().valueToProportionOfLength (sizeKnob->getSlider().getValue())
                 == Catch::Approx (testCase.hostNormalised));
        REQUIRE (sizeKnob->getDisplayValue() == testCase.expectedText);
    }
}
