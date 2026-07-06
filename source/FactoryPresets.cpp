#include "FactoryPresets.h"
#include "ParameterIDs.h"

namespace sendbloom
{

namespace
{

struct PresetDef
{
    const char* name;
    float inputGain;
    float inputThreshold;
    float size;
    float level;
    float distn;
    float outputGain;
    bool darkMode;
    int gatePrePost;
    bool sendConnected;
    float sendAmount;
    int sendFeel;
    bool authenticColor;
    bool extendedStereo;
    bool dirtOs;
    bool bypass;
};

constexpr std::array<PresetDef, FactoryPresets::kNumPresets> kPresets { {
    { "Sparkle Verb",     0.55f, 0.30f, 0.35f, 0.60f, 0.10f, 0.0f,  false, 1, false, 0.8f, 0, true,  false, false, false },
    { "Cut Sample Gate",  0.60f, 0.55f, 0.25f, 0.55f, 0.20f, 0.0f,  false, 1, false, 0.7f, 0, true,  false, false, false },
    { "Spacerock Burn",   0.65f, 0.35f, 0.80f, 0.70f, 0.55f, 1.0f,  true,  1, false, 0.9f, 0, true,  false, false, false },
    { "Dry Dub Sends",    0.50f, 0.40f, 0.48f, 0.25f, 0.15f, -1.0f, false, 0, false, 0.6f, 0, true,  false, false, false },
    { "Dark Bloom",       0.55f, 0.38f, 0.55f, 0.65f, 0.30f, 0.0f,  true,  0, false, 0.75f, 1, true,  false, false, false },
    { "Firm Pressure",    0.58f, 0.32f, 0.45f, 0.60f, 0.25f, 0.0f,  false, 0, true,  0.85f, 0, true,  false, false, false },
    { "Gated Room",       0.52f, 0.42f, 0.75f, 0.68f, 0.20f, 0.5f,  false, 1, false, 0.7f, 1, true,  false, false, false },
    { "Hot Clip",         0.92f, 0.28f, 0.40f, 0.72f, 0.45f, 2.0f,  false, 1, true,  1.0f, 0, false, false, false, false },
} };

void setNorm (juce::AudioProcessorValueTreeState& apvts, const char* id, float norm)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (norm);
}

void setDenorm (juce::AudioProcessorValueTreeState& apvts, const char* id, float denorm)
{
    if (auto* param = apvts.getParameter (id))
        param->setValueNotifyingHost (param->convertTo0to1 (denorm));
}

void applyDef (juce::AudioProcessorValueTreeState& apvts, const PresetDef& def)
{
    using namespace ParameterIDs;

    setDenorm (apvts, inputGain, def.inputGain);
    setDenorm (apvts, inputThreshold, def.inputThreshold);
    setDenorm (apvts, size, def.size);
    setDenorm (apvts, level, def.level);
    setDenorm (apvts, distn, def.distn);
    setDenorm (apvts, outputGain, def.outputGain);
    setNorm (apvts, darkMode, def.darkMode ? 1.0f : 0.0f);
    setNorm (apvts, gatePrePost, def.gatePrePost == 1 ? 1.0f : 0.0f);
    setNorm (apvts, sendConnected, def.sendConnected ? 1.0f : 0.0f);
    setDenorm (apvts, sendAmount, def.sendAmount);
    setNorm (apvts, sendFeel, def.sendFeel == 1 ? 1.0f : 0.0f);
    setNorm (apvts, authenticColor, def.authenticColor ? 1.0f : 0.0f);
    setNorm (apvts, extendedStereo, def.extendedStereo ? 1.0f : 0.0f);
    setNorm (apvts, dirtOs, def.dirtOs ? 1.0f : 0.0f);
    setNorm (apvts, bypass, def.bypass ? 1.0f : 0.0f);
}

juce::ValueTree makeParamChild (const char* id, float denormalised, juce::RangedAudioParameter* param)
{
    juce::ValueTree child { "PARAM" };
    child.setProperty ("id", id, nullptr);
    child.setProperty ("value", denormalised, nullptr);
    juce::ignoreUnused (param);
    return child;
}

} // namespace

juce::String FactoryPresets::getPresetName (int index)
{
    if (index < 0 || index >= kNumPresets)
        return {};

    return kPresets[static_cast<size_t> (index)].name;
}

juce::ValueTree FactoryPresets::makePresetState (int index)
{
    if (index < 0 || index >= kNumPresets)
        return {};

    const auto& def = kPresets[static_cast<size_t> (index)];
    juce::ValueTree state { "SendBloomParams" };

    using namespace ParameterIDs;
    state.appendChild (makeParamChild (inputGain, def.inputGain, nullptr), nullptr);
    state.appendChild (makeParamChild (inputThreshold, def.inputThreshold, nullptr), nullptr);
    state.appendChild (makeParamChild (size, def.size, nullptr), nullptr);
    state.appendChild (makeParamChild (level, def.level, nullptr), nullptr);
    state.appendChild (makeParamChild (distn, def.distn, nullptr), nullptr);
    state.appendChild (makeParamChild (outputGain, def.outputGain, nullptr), nullptr);
    state.appendChild (makeParamChild (darkMode, def.darkMode ? 1.0f : 0.0f, nullptr), nullptr);
    state.appendChild (makeParamChild (gatePrePost, static_cast<float> (def.gatePrePost), nullptr), nullptr);
    state.appendChild (makeParamChild (sendConnected, def.sendConnected ? 1.0f : 0.0f, nullptr), nullptr);
    state.appendChild (makeParamChild (sendAmount, def.sendAmount, nullptr), nullptr);
    state.appendChild (makeParamChild (sendFeel, static_cast<float> (def.sendFeel), nullptr), nullptr);
    state.appendChild (makeParamChild (authenticColor, def.authenticColor ? 1.0f : 0.0f, nullptr), nullptr);
    state.appendChild (makeParamChild (extendedStereo, def.extendedStereo ? 1.0f : 0.0f, nullptr), nullptr);
    state.appendChild (makeParamChild (dirtOs, def.dirtOs ? 1.0f : 0.0f, nullptr), nullptr);
    state.appendChild (makeParamChild (bypass, def.bypass ? 1.0f : 0.0f, nullptr), nullptr);

    return state;
}

void FactoryPresets::applyPreset (juce::AudioProcessorValueTreeState& apvts, int index)
{
    if (index < 0 || index >= kNumPresets)
        return;

    applyDef (apvts, kPresets[static_cast<size_t> (index)]);
}

} // namespace sendbloom
