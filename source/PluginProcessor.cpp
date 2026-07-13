#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterLayout.h"
#include "ParameterIDs.h"
#include "FactoryPresets.h"
#include "ParameterSnapshot.h"
#include "ParameterCurves.h"
#include "BypassCrossfade.h"
#include "ParallelWetMixer.h"
#include "OutputStage.h"
#include "SafeXml.h"

#include <cmath>
#include <optional>

namespace sendbloom
{

namespace
{

constexpr auto kLegacyAuthenticColorId = "authentic_color";
const juce::Identifier kProgramIndexProperty { "program_index" };
const juce::Identifier kProgramCustomProperty { "program_custom" };

juce::ValueTree stateForProgram (int programIndex)
{
    if (programIndex == 0)
        return FactoryPresets::makeInitState();

    return FactoryPresets::makePresetState (programIndex - 1);
}

std::optional<float> findParameterValue (const juce::ValueTree& state,
                                         const juce::String& parameterID)
{
    if (state.getProperty ("id").toString() == parameterID && state.hasProperty ("value"))
        return state.getProperty ("value").toString().getFloatValue();

    for (int i = 0; i < state.getNumChildren(); ++i)
        if (const auto value = findParameterValue (state.getChild (i), parameterID))
            return value;

    return std::nullopt;
}

bool stateMatchesProgram (const juce::ValueTree& state, int programIndex)
{
    const auto expected = stateForProgram (programIndex);

    if (! state.isValid() || ! expected.isValid())
        return false;

    for (const auto* id : ParameterIDs::all)
    {
        const auto actualValue = findParameterValue (state, id);
        const auto expectedValue = findParameterValue (expected, id);

        if (! actualValue.has_value() || ! expectedValue.has_value()
            || std::abs (*actualValue - *expectedValue) > 1.0e-5f)
            return false;
    }

    return true;
}

int findMatchingProgram (const juce::ValueTree& state)
{
    constexpr int kNumPrograms = FactoryPresets::kNumPresets + 1;

    for (int program = 0; program < kNumPrograms; ++program)
        if (stateMatchesProgram (state, program))
            return program;

    return -1;
}

void removeRetiredParameters (juce::ValueTree state)
{
    for (int i = state.getNumChildren(); --i >= 0;)
    {
        auto child = state.getChild (i);

        if (child.getProperty ("id").toString() == kLegacyAuthenticColorId)
            state.removeChild (i, nullptr);
        else
            removeRetiredParameters (child);
    }
}

} // namespace

PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "SendBloomParams", createParameterLayout())
{
    setLatencySamples (0);

    for (const auto* id : ParameterIDs::all)
        apvts.addParameterListener (id, this);
}

PluginProcessor::~PluginProcessor()
{
    for (const auto* id : ParameterIDs::all)
        apvts.removeParameterListener (id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    return sendbloom::createParameterLayout();
}

juce::AudioProcessorParameter* PluginProcessor::getBypassParameter() const
{
    return apvts.getParameter (ParameterIDs::bypass);
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    const auto sizeNorm = apvts.getRawParameterValue (ParameterIDs::size)->load();
    const auto rt60 = ParameterCurves::sizeToRT60 (sizeNorm);
    const auto darkMode = apvts.getRawParameterValue (ParameterIDs::darkMode)->load() > 0.5f;
    constexpr auto kDarkPredelaySeconds = 0.055f;
    return static_cast<double> (rt60 + (darkMode ? kDarkPredelaySeconds : 0.0f));
}

int PluginProcessor::getNumPrograms()
{
    return FactoryPresets::kNumPresets + 1;
}

int PluginProcessor::getCurrentProgram()
{
    return currentProgramIndex_.load();
}

void PluginProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= getNumPrograms())
        return;

    programStateUpdateInProgress_.store (true);
    const auto applied = index == 0 ? FactoryPresets::applyInit (apvts)
                                    : FactoryPresets::applyPreset (apvts, index - 1);
    programStateUpdateInProgress_.store (false);

    if (! applied)
        return;

    currentProgramIndex_.store (index);
    currentProgramCustom_.store (false);
    smoothedBank.setTargets (ParameterSnapshot::capture (apvts));
}

const juce::String PluginProcessor::getProgramName (int index)
{
    if (index == 0)
        return "Init";

    return FactoryPresets::getPresetName (index - 1);
}

juce::String PluginProcessor::getCurrentProgramDisplayName() const
{
    if (currentProgramCustom_.load())
        return "Custom";

    const auto index = currentProgramIndex_.load();
    return index == 0 ? juce::String ("Init") : FactoryPresets::getPresetName (index - 1);
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    smoothedBank.prepare (sampleRate);
    pressureController.prepare (sampleRate);
    chain.prepare (sampleRate, samplesPerBlock);
    inputStage.prepare (sampleRate);
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    preparedMaxBlock_ = samplesPerBlock;
    monoScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    envelopeScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    wetScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    wetGainScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    sendGainScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    distnScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    thresholdLinearScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    bypassWetScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    outputGainScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
    smoothedBank.setTargets (ParameterSnapshot::capture (apvts));
    smoothedBank.snapToTargets();
    {
        const auto snap = ParameterSnapshot::capture (apvts);
        pressureController.setConnected (snap.sendConnected);
        pressureController.setHostPressureTarget (snap.sendAmountNorm);
        pressureController.setMidiPressureTarget (0.0f);
        pressureController.setFirmFeel (snap.sendFirmFeel);
        pressureController.snapToTarget();
    }
}

void PluginProcessor::releaseResources()
{
    preparedMaxBlock_ = 0;
    pressureController.reset();
    monoScratch_.clear();
    envelopeScratch_.clear();
    wetScratch_.clear();
    wetGainScratch_.clear();
    sendGainScratch_.clear();
    distnScratch_.clear();
    thresholdLinearScratch_.clear();
    bypassWetScratch_.clear();
    outputGainScratch_.clear();
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

void PluginProcessor::applyPressureMidiAtSample (const juce::MidiBuffer& midiMessages,
                                                 int samplePosition) noexcept
{
    // ADR-V1-03: MIDI is realtime modulation only — never touch APVTS / host notify.
    bool resetControllers = false;
    std::optional<float> cc1Value;

    for (const auto metadata : midiMessages)
    {
        if (metadata.samplePosition != samplePosition)
            continue;

        const auto message = metadata.getMessage();

        if (message.isResetAllControllers())
            resetControllers = true;
        else if (message.isController() && message.getControllerNumber() == 1)
            cc1Value = static_cast<float> (message.getControllerValue()) / 127.0f;
    }

    if (resetControllers)
        pressureController.setMidiPressureTarget (0.0f);
    else if (cc1Value.has_value())
        pressureController.setMidiPressureTarget (*cc1Value);
}

int PluginProcessor::findNextPressureMidiSampleAfter (const juce::MidiBuffer& midiMessages,
                                                      int afterSample,
                                                      int numSamples) const noexcept
{
    int next = numSamples;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isResetAllControllers()
            && (! message.isController() || message.getControllerNumber() != 1))
            continue;

        if (metadata.samplePosition > afterSample && metadata.samplePosition < next)
            next = metadata.samplePosition;
    }

    return next;
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // RT-15 / D-05: never index prepare-sized scratch when unprepared.
    if (preparedMaxBlock_ <= 0)
        return;

    const auto snap = ParameterSnapshot::capture (apvts);
    smoothedBank.setTargets (snap);
    pressureController.setConnected (snap.sendConnected);
    pressureController.setHostPressureTarget (snap.sendAmountNorm);
    // MIDI target persists across blocks until a CC1 event updates it (ADR-V1-03).
    pressureController.setFirmFeel (snap.sendFirmFeel);

    const auto numSamples = buffer.getNumSamples();
    int offset = 0;

    while (offset < numSamples)
    {
        // RT-04 / MIDI-04/05: apply CC1 at this sample, then cut span before the next CC1.
        applyPressureMidiAtSample (midiMessages, offset);

        const auto remaining = numSamples - offset;
        const auto nextPressureMidi = findNextPressureMidiSampleAfter (midiMessages, offset, numSamples);
        const auto distanceToNextPressureMidi = nextPressureMidi - offset;
        const auto span = juce::jmin (remaining, preparedMaxBlock_, kControlQuantum,
                                     distanceToNextPressureMidi);
        processSpan (buffer, offset, span, snap);
        offset += span;
    }

    clipHoldFlag.store (inputStage.isClipHoldActive());
}

void PluginProcessor::processSpan (juce::AudioBuffer<float>& buffer,
                                   int offset,
                                   int span,
                                   const ParameterSnapshot& snap) noexcept
{
    jassert (span > 0);
    jassert (span <= preparedMaxBlock_);
    jassert (span <= kControlQuantum);
    jassert (offset >= 0);
    jassert (offset + span <= buffer.getNumSamples());

    const auto numChannels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());
    const auto gatePreSoft = snap.gatePreSoft;
    const auto extendedStereo = snap.extendedStereo;

    for (int channel = 0; channel < numChannels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, offset, span);

    float spanRt60 = 0.0f;
    float spanDark = 0.0f;
    for (int sample = 0; sample < span; ++sample)
    {
        const auto inputGain = smoothedBank.getNextInputGainLinear();
        const auto sizeNorm = smoothedBank.getNextSizeNorm();
        const auto distnBlend = smoothedBank.getNextDistnBlend();
        wetGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextLevelWetGain();
        sendGainScratch_[static_cast<size_t> (sample)] = pressureController.processSample();
        distnScratch_[static_cast<size_t> (sample)] = distnBlend;
        thresholdLinearScratch_[static_cast<size_t> (sample)] =
            smoothedBank.getNextInputThresholdLinear();
        bypassWetScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextBypassWetMix();
        outputGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextOutputGainLinear();
        const auto darkModeMix = smoothedBank.getNextDarkModeTarget();
        if (sample == 0)
        {
            spanRt60 = ParameterCurves::sizeToRT60 (sizeNorm);
            spanDark = darkModeMix;
        }

        float monoSum = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
            monoSum += dryBuffer.getReadPointer (channel)[sample];

        monoSum /= static_cast<float> (juce::jmax (1, numChannels));
        monoScratch_[static_cast<size_t> (sample)] = inputStage.processSample (monoSum, inputGain);
        envelopeScratch_[static_cast<size_t> (sample)] =
            chain.getEnvelope().process (std::abs (monoScratch_[static_cast<size_t> (sample)]));
    }

    // ADR-V1-06 / RT-06: send, distn, and threshold consumed per sample.
    chain.processBlock (monoScratch_.data(), envelopeScratch_.data(), wetScratch_.data(), span,
                        spanRt60, spanDark,
                        distnScratch_.data(), sendGainScratch_.data(), thresholdLinearScratch_.data(),
                        gatePreSoft);

    // ADR-V1-10: build output-gained engaged path first, then crossfade against
    // original per-channel dry. Never apply OutputStage after the bypass mix.
    for (int sample = 0; sample < span; ++sample)
    {
        const auto wetGain = wetGainScratch_[static_cast<size_t> (sample)];
        const auto engagedMix = bypassWetScratch_[static_cast<size_t> (sample)];
        const auto outputGain = outputGainScratch_[static_cast<size_t> (sample)];
        const auto wet = wetScratch_[static_cast<size_t> (sample)];
        const auto outIndex = offset + sample;

        if (extendedStereo)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
                const auto mixed = ParallelWetMixer::mix (dryTap, wet, wetGain);
                const auto engaged = OutputStage::processSample (mixed, outputGain);
                buffer.getWritePointer (channel)[outIndex] =
                    BypassCrossfade::mixSample (dryTap, engaged, engagedMix);
            }
        }
        else
        {
            // Engaged path stays mono-first dual-mono (CORE-18); bypass dry is per-channel.
            float monoSum = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
                monoSum += dryBuffer.getReadPointer (channel)[sample];

            monoSum /= static_cast<float> (juce::jmax (1, numChannels));
            const auto mixed = ParallelWetMixer::mix (monoSum, wet, wetGain);
            const auto engaged = OutputStage::processSample (mixed, outputGain);

            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
                buffer.getWritePointer (channel)[outIndex] =
                    BypassCrossfade::mixSample (dryTap, engaged, engagedMix);
            }
        }
    }
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    const auto programIndex = currentProgramIndex_.load();
    const auto isCustom = currentProgramCustom_.load() || ! stateMatchesProgram (state, programIndex);
    state.setProperty (kProgramIndexProperty, programIndex, nullptr);
    state.setProperty (kProgramCustomProperty, isCustom, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // Host-provided state is untrusted. JUCE 8.0.12's generic XmlDocument
    // accepts DTD/entity declarations, so reject them and bound input before
    // getXmlFromBinary reaches the parser.
    if (sizeInBytes <= 0
        || ! SafeXml::acceptsDocument (data, static_cast<size_t> (sizeInBytes)))
        return;

    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto state = juce::ValueTree::fromXml (*xml);
            removeRetiredParameters (state);

            const auto hasProgramMetadata = state.hasProperty (kProgramIndexProperty);
            auto programIndex = hasProgramMetadata
                                  ? static_cast<int> (state.getProperty (kProgramIndexProperty))
                                  : findMatchingProgram (state);
            const auto storedCustom = static_cast<bool> (
                state.getProperty (kProgramCustomProperty, false));

            if (programIndex < 0 || programIndex >= getNumPrograms())
                programIndex = 0;

            const auto isCustom = storedCustom || ! stateMatchesProgram (state, programIndex);

            programStateUpdateInProgress_.store (true);
            apvts.replaceState (state);
            programStateUpdateInProgress_.store (false);
            currentProgramIndex_.store (programIndex);
            currentProgramCustom_.store (isCustom);
            smoothedBank.setTargets (ParameterSnapshot::capture (apvts));
        }
}

void PluginProcessor::parameterChanged (const juce::String&, float)
{
    if (! programStateUpdateInProgress_.load())
        currentProgramCustom_.store (true);
}

} // namespace sendbloom

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sendbloom::PluginProcessor();
}
