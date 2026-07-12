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

namespace sendbloom
{

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
}

PluginProcessor::~PluginProcessor() = default;

void PluginProcessor::updateReportedLatency (bool /* targetAuthenticOn */) noexcept
{
    // ADR-003 Path B: always report zero host PDC (CHN-04). Wet-only ProperSRC
    // delay (~3.9–4.1 ms) is accepted as musically fine for parallel reverb.
    setLatencySamples (0);
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
    return FactoryPresets::kNumPresets;
}

int PluginProcessor::getCurrentProgram()
{
    return currentProgramIndex;
}

void PluginProcessor::setCurrentProgram (int index)
{
    if (index < 0 || index >= FactoryPresets::kNumPresets)
        return;

    currentProgramIndex = index;
    FactoryPresets::applyPreset (apvts, index);
    smoothedBank.setTargets (ParameterSnapshot::capture (apvts));
}

const juce::String PluginProcessor::getProgramName (int index)
{
    return FactoryPresets::getPresetName (index);
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
    thresholdDbScratch_.assign (static_cast<size_t> (samplesPerBlock), 0.0f);
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
    requestedAuthenticColor_ =
        apvts.getRawParameterValue (ParameterIDs::authenticColor)->load() > 0.5f;
    updateReportedLatency (requestedAuthenticColor_);
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
    thresholdDbScratch_.clear();
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

void PluginProcessor::applyCc1AtSample (const juce::MidiBuffer& midiMessages,
                                        int samplePosition,
                                        bool connected) noexcept
{
    // ADR-V1-03: MIDI is realtime modulation only — never touch APVTS / host notify.
    if (! connected)
        return;

    for (const auto metadata : midiMessages)
    {
        if (metadata.samplePosition != samplePosition)
            continue;

        const auto message = metadata.getMessage();

        if (! message.isController() || message.getControllerNumber() != 1)
            continue;

        const auto norm = static_cast<float> (message.getControllerValue()) / 127.0f;
        pressureController.setMidiPressureTarget (norm);
    }
}

int PluginProcessor::findNextCc1SampleAfter (const juce::MidiBuffer& midiMessages,
                                             int afterSample,
                                             int numSamples) const noexcept
{
    int next = numSamples;

    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (! message.isController() || message.getControllerNumber() != 1)
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

    // ADR-V1-07 / RT-08…11: one engine-crossfade request per authentic snapshot edge.
    if (snap.authenticColor != requestedAuthenticColor_)
    {
        chain.requestEngineCrossfade (snap.authenticColor);
        updateReportedLatency (snap.authenticColor);
        requestedAuthenticColor_ = snap.authenticColor;
    }

    const auto numSamples = buffer.getNumSamples();
    const auto connected = snap.sendConnected;
    int offset = 0;

    while (offset < numSamples)
    {
        // RT-04 / MIDI-04/05: apply CC1 at this sample, then cut span before the next CC1.
        applyCc1AtSample (midiMessages, offset, connected);

        const auto remaining = numSamples - offset;
        const auto nextCc1 = findNextCc1SampleAfter (midiMessages, offset, numSamples);
        const auto distanceToNextCc1 = nextCc1 - offset;
        const auto span = juce::jmin (remaining, preparedMaxBlock_, kControlQuantum, distanceToNextCc1);
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
    bool spanAuthentic = snap.authenticColor;

    for (int sample = 0; sample < span; ++sample)
    {
        const auto inputGain = smoothedBank.getNextInputGainLinear();
        const auto sizeNorm = smoothedBank.getNextSizeNorm();
        const auto distnBlend = smoothedBank.getNextDistnBlend();
        wetGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextLevelWetGain();
        sendGainScratch_[static_cast<size_t> (sample)] = pressureController.processSample();
        distnScratch_[static_cast<size_t> (sample)] = distnBlend;
        const auto thresholdNorm = smoothedBank.getNextInputThresholdNorm();
        thresholdDbScratch_[static_cast<size_t> (sample)] =
            ParameterCurves::inputThresholdDb (thresholdNorm);
        bypassWetScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextBypassWetMix();
        outputGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextOutputGainLinear();
        (void) smoothedBank.getNextLevelDryGain();
        const auto darkModeMix = smoothedBank.getNextDarkModeTarget();
        // authenticColorTarget smoother is no longer the request trigger (ADR-V1-07).
        (void) smoothedBank.getNextAuthenticColorTarget();

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
                        spanRt60, spanDark, spanAuthentic,
                        distnScratch_.data(), sendGainScratch_.data(), thresholdDbScratch_.data(),
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
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

} // namespace sendbloom

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new sendbloom::PluginProcessor();
}
