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
    lastAuthenticColorSmoothed_ =
        apvts.getRawParameterValue (ParameterIDs::authenticColor)->load() > 0.5f ? 1.0f : 0.0f;

    const auto authenticOn =
        apvts.getRawParameterValue (ParameterIDs::authenticColor)->load() > 0.5f;
    updateReportedLatency (authenticOn);
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

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer,
                                    juce::MidiBuffer& midiMessages)
{
    for (const auto metadata : midiMessages)
    {
        const auto message = metadata.getMessage();

        if (message.isController() && message.getControllerNumber() == 1)
        {
            if (apvts.getRawParameterValue (ParameterIDs::sendConnected)->load() > 0.5f)
            {
                if (auto* sendParam = apvts.getRawParameterValue (ParameterIDs::sendAmount))
                {
                    const auto norm = static_cast<float> (message.getControllerValue()) / 127.0f;
                    sendParam->store (norm);
                }
            }
        }
    }

    juce::ScopedNoDenormals noDenormals;
    const auto totalNumInputChannels  = getTotalNumInputChannels();
    const auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    const auto snap = ParameterSnapshot::capture (apvts);
    smoothedBank.setTargets (snap);
    pressureController.setConnected (snap.sendConnected);
    pressureController.setHostPressureTarget (snap.sendAmountNorm);
    pressureController.setMidiPressureTarget (0.0f);
    pressureController.setFirmFeel (snap.sendFirmFeel);

    const auto numSamples = buffer.getNumSamples();
    const auto numOutputChannels = getTotalNumOutputChannels();

    if (dryBuffer.getNumChannels() < numOutputChannels || dryBuffer.getNumSamples() < numSamples)
        dryBuffer.setSize (numOutputChannels, numSamples, false, false, true);

    const auto numChannels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());
    const auto gatePreSoft = snap.gatePreSoft;
    const auto extendedStereo = snap.extendedStereo;

    for (int channel = 0; channel < numChannels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    if (numSamples > preparedMaxBlock_)
    {
        for (int sample = 0; sample < numSamples; ++sample)
        {
            const auto inputGain = smoothedBank.getNextInputGainLinear();
            const auto sizeNorm = smoothedBank.getNextSizeNorm();
            const auto distnBlend = smoothedBank.getNextDistnBlend();
            const auto wetGain = smoothedBank.getNextLevelWetGain();
            (void) pressureController.processSample();
            const auto thresholdNorm = smoothedBank.getNextInputThresholdNorm();
            const auto bypassWet = smoothedBank.getNextBypassWetMix();
            const auto outputGain = smoothedBank.getNextOutputGainLinear();

            (void) smoothedBank.getNextLevelDryGain();
            const auto darkModeMix = smoothedBank.getNextDarkModeTarget();
            (void) smoothedBank.getNextAuthenticColorTarget();

            const auto rt60 = ParameterCurves::sizeToRT60 (sizeNorm);
            const auto thresholdDb = ParameterCurves::inputThresholdDb (thresholdNorm);
            const auto dryMix = 1.0f - bypassWet;

            float monoSum = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
                monoSum += dryBuffer.getReadPointer (channel)[sample];

            monoSum /= static_cast<float> (numChannels);
            (void) inputStage.processSample (monoSum, inputGain);
            (void) chain.getEnvelope().process (std::abs (monoSum));
            (void) rt60;
            (void) darkModeMix;
            (void) distnBlend;
            (void) thresholdDb;

            constexpr auto wet = 0.0f;

            if (extendedStereo)
            {
                for (int channel = 0; channel < numChannels; ++channel)
                {
                    const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
                    const auto mixed = ParallelWetMixer::mix (dryTap, wet, wetGain);
                    const auto preOutput = dryTap * dryMix + mixed * bypassWet;
                    buffer.getWritePointer (channel)[sample] = OutputStage::processSample (preOutput, outputGain);
                }
            }
            else
            {
                const auto mixed = ParallelWetMixer::mix (monoSum, wet, wetGain);
                const auto preOutput = monoSum * dryMix + mixed * bypassWet;
                const auto out = OutputStage::processSample (preOutput, outputGain);

                for (int channel = 0; channel < numChannels; ++channel)
                    buffer.getWritePointer (channel)[sample] = out;
            }
        }

        clipHoldFlag.store (inputStage.isClipHoldActive());
        return;
    }

    float blockStartRt60 = 0.0f;
    float blockStartDark = 0.0f;
    bool blockStartAuthentic = false;
    float blockStartDistn = 0.0f;
    float blockStartThresholdDb = 0.0f;
    float prevAuthenticSmoothed = lastAuthenticColorSmoothed_;
    bool crossfadeEdgeHandled = false;

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto inputGain = smoothedBank.getNextInputGainLinear();
        const auto sizeNorm = smoothedBank.getNextSizeNorm();
        const auto distnBlend = smoothedBank.getNextDistnBlend();
        wetGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextLevelWetGain();
        sendGainScratch_[static_cast<size_t> (sample)] = pressureController.processSample();
        const auto thresholdNorm = smoothedBank.getNextInputThresholdNorm();
        bypassWetScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextBypassWetMix();
        outputGainScratch_[static_cast<size_t> (sample)] = smoothedBank.getNextOutputGainLinear();
        (void) smoothedBank.getNextLevelDryGain();
        const auto darkModeMix = smoothedBank.getNextDarkModeTarget();
        const auto authenticColorTarget = smoothedBank.getNextAuthenticColorTarget();
        const auto authenticColor = authenticColorTarget > 0.5f;

        if (! crossfadeEdgeHandled
            && ((prevAuthenticSmoothed <= 0.5f && authenticColorTarget > 0.5f)
                || (prevAuthenticSmoothed > 0.5f && authenticColorTarget <= 0.5f)))
        {
            chain.requestEngineCrossfade (authenticColorTarget > 0.5f);
            updateReportedLatency (authenticColorTarget > 0.5f);
            crossfadeEdgeHandled = true;
        }

        prevAuthenticSmoothed = authenticColorTarget;

        if (sample == 0)
        {
            blockStartRt60 = ParameterCurves::sizeToRT60 (sizeNorm);
            blockStartDark = darkModeMix;
            blockStartAuthentic = authenticColor;
            blockStartDistn = distnBlend;
            blockStartThresholdDb = ParameterCurves::inputThresholdDb (thresholdNorm);
        }

        float monoSum = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
            monoSum += dryBuffer.getReadPointer (channel)[sample];

        monoSum /= static_cast<float> (numChannels);
        monoScratch_[static_cast<size_t> (sample)] = inputStage.processSample (monoSum, inputGain);
        envelopeScratch_[static_cast<size_t> (sample)] =
            chain.getEnvelope().process (std::abs (monoScratch_[static_cast<size_t> (sample)]));
    }

    lastAuthenticColorSmoothed_ = prevAuthenticSmoothed;

    chain.processBlock (monoScratch_.data(), envelopeScratch_.data(), wetScratch_.data(), numSamples,
                        blockStartRt60, blockStartDark, blockStartAuthentic, blockStartDistn,
                        sendGainScratch_.data(), gatePreSoft, blockStartThresholdDb);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto wetGain = wetGainScratch_[static_cast<size_t> (sample)];
        const auto bypassWet = bypassWetScratch_[static_cast<size_t> (sample)];
        const auto outputGain = outputGainScratch_[static_cast<size_t> (sample)];

        const auto dryMix = 1.0f - bypassWet;
        const auto wet = wetScratch_[static_cast<size_t> (sample)];

        if (extendedStereo)
        {
            for (int channel = 0; channel < numChannels; ++channel)
            {
                const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
                const auto mixed = ParallelWetMixer::mix (dryTap, wet, wetGain);
                const auto preOutput = dryTap * dryMix + mixed * bypassWet;
                buffer.getWritePointer (channel)[sample] = OutputStage::processSample (preOutput, outputGain);
            }
        }
        else
        {
            float monoSum = 0.0f;

            for (int channel = 0; channel < numChannels; ++channel)
                monoSum += dryBuffer.getReadPointer (channel)[sample];

            monoSum /= static_cast<float> (numChannels);
            const auto mixed = ParallelWetMixer::mix (monoSum, wet, wetGain);
            const auto preOutput = monoSum * dryMix + mixed * bypassWet;
            const auto out = OutputStage::processSample (preOutput, outputGain);

            for (int channel = 0; channel < numChannels; ++channel)
                buffer.getWritePointer (channel)[sample] = out;
        }
    }

    clipHoldFlag.store (inputStage.isClipHoldActive());
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
