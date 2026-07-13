#include "FactoryPresets.h"
#include "ParameterIDs.h"
#include "SafeXml.h"
#include <BinaryData.h>

namespace sendbloom
{

namespace
{

struct PresetResource
{
    const char* name;
    const char* xml;
    int xmlSize;
};

const std::array<PresetResource, FactoryPresets::kNumPresets> kPresetResources { {
    { "Sparkle Verb",    BinaryData::Sparkle_Verb_xml,    BinaryData::Sparkle_Verb_xmlSize },
    { "Cut Sample Gate", BinaryData::Cut_Sample_Gate_xml, BinaryData::Cut_Sample_Gate_xmlSize },
    { "Spacerock Burn",  BinaryData::Spacerock_Burn_xml,  BinaryData::Spacerock_Burn_xmlSize },
    { "Dry Dub Sends",   BinaryData::Dry_Dub_Sends_xml,   BinaryData::Dry_Dub_Sends_xmlSize },
    { "Dark Bloom",      BinaryData::Dark_Bloom_xml,      BinaryData::Dark_Bloom_xmlSize },
    { "Firm Pressure",   BinaryData::Firm_Pressure_xml,   BinaryData::Firm_Pressure_xmlSize },
    { "Gated Room",      BinaryData::Gated_Room_xml,      BinaryData::Gated_Room_xmlSize },
    { "Hot Clip",        BinaryData::Hot_Clip_xml,        BinaryData::Hot_Clip_xmlSize },
} };

juce::ValueTree parseEmbeddedXml (const char* data, int size)
{
    const auto xml = SafeXml::parseDocument (data, static_cast<size_t> (size));

    if (xml == nullptr)
        return {};

    return juce::ValueTree::fromXml (*xml);
}

bool containsParameter (const juce::ValueTree& state, const juce::String& parameterID)
{
    if (state.getProperty ("id").toString() == parameterID && state.hasProperty ("value"))
        return true;

    for (int i = 0; i < state.getNumChildren(); ++i)
        if (containsParameter (state.getChild (i), parameterID))
            return true;

    return false;
}

} // namespace

juce::ValueTree FactoryPresets::makeInitState()
{
    return parseEmbeddedXml (BinaryData::Init_xml, BinaryData::Init_xmlSize);
}

juce::String FactoryPresets::getPresetName (int index)
{
    if (index < 0 || index >= kNumPresets)
        return {};

    return kPresetResources[static_cast<size_t> (index)].name;
}

juce::ValueTree FactoryPresets::makePresetState (int index)
{
    if (index < 0 || index >= kNumPresets)
        return {};

    const auto& preset = kPresetResources[static_cast<size_t> (index)];
    return parseEmbeddedXml (preset.xml, preset.xmlSize);
}

bool FactoryPresets::applyState (juce::AudioProcessorValueTreeState& apvts,
                                 const juce::ValueTree& state)
{
    if (! state.isValid() || state.getType() != apvts.state.getType())
        return false;

    for (const auto* id : ParameterIDs::all)
        if (! containsParameter (state, id))
            return false;

    apvts.replaceState (state.createCopy());
    return true;
}

bool FactoryPresets::applyInit (juce::AudioProcessorValueTreeState& apvts)
{
    return applyState (apvts, makeInitState());
}

bool FactoryPresets::applyPreset (juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= kNumPresets)
        return false;

    return applyState (apvts, makePresetState (index));
}

} // namespace sendbloom
