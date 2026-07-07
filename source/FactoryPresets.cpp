#include "FactoryPresets.h"
#include "ParameterIDs.h"
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

bool applyEmbeddedXml (juce::AudioProcessorValueTreeState& apvts, const PresetResource& preset)
{
    const auto xml = juce::parseXML (juce::String (preset.xml, static_cast<size_t> (preset.xmlSize)));

    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType()))
        return false;

    apvts.replaceState (juce::ValueTree::fromXml (*xml));
    return true;
}

} // namespace

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

    const auto xml = juce::parseXML (juce::String (kPresetResources[static_cast<size_t> (index)].xml,
                                                   static_cast<size_t> (kPresetResources[static_cast<size_t> (index)].xmlSize)));

    if (xml == nullptr)
        return {};

    return juce::ValueTree::fromXml (*xml);
}

void FactoryPresets::applyPreset (juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= kNumPresets)
        return;

    applyEmbeddedXml (apvts, kPresetResources[static_cast<size_t> (index)]);
}

} // namespace sendbloom
