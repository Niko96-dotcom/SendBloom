#include <ParameterLayout.h>
#include <ParameterIDs.h>
#include <PluginProcessor.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace
{

const juce::AudioParameterFloat* findFloatParam (sendbloom::PluginProcessor& plugin, const char* paramId)
{
    if (auto* param = plugin.getAPVTS().getParameter (paramId))
        return dynamic_cast<juce::AudioParameterFloat*> (param);

    return nullptr;
}

} // namespace

TEST_CASE ("createParameterLayout exposes 15 parameters", "[parm][layout]")
{
    sendbloom::PluginProcessor plugin;
    REQUIRE (plugin.getParameters().size() == 15);
}

TEST_CASE ("size and output_gain ranges and defaults", "[parm][layout]")
{
    sendbloom::PluginProcessor plugin;

    const auto* sizeParam = findFloatParam (plugin, sendbloom::ParameterIDs::size);
    REQUIRE (sizeParam != nullptr);
    const auto sizeRange = sizeParam->getNormalisableRange();
    REQUIRE (sizeRange.start == Catch::Approx (0.0f));
    REQUIRE (sizeRange.end == Catch::Approx (1.0f));
    REQUIRE (plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::size)->load()
             == Catch::Approx (0.5f));

    const auto* outParam = findFloatParam (plugin, sendbloom::ParameterIDs::outputGain);
    REQUIRE (outParam != nullptr);
    const auto outRange = outParam->getNormalisableRange();
    REQUIRE (outRange.start == Catch::Approx (-12.0f));
    REQUIRE (outRange.end == Catch::Approx (12.0f));
    REQUIRE (plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::outputGain)->load()
             == Catch::Approx (0.0f));

    REQUIRE (plugin.getAPVTS().getRawParameterValue (sendbloom::ParameterIDs::gatePrePost)->load()
             == Catch::Approx (1.0f));
}
