#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterLayout.h"
#include "ParameterIDs.h"
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
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    smoothedBank.prepare (sampleRate);
    chain.prepare (sampleRate, samplesPerBlock);
    inputStage.prepare (sampleRate);
    dryBuffer.setSize (getTotalNumOutputChannels(), samplesPerBlock);
    smoothedBank.setTargets (ParameterSnapshot::capture (apvts));
    smoothedBank.snapToTargets();
}

void PluginProcessor::releaseResources()
{
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
                if (auto* sendParam = apvts.getParameter (ParameterIDs::sendAmount))
                {
                    const auto norm = static_cast<float> (message.getControllerValue()) / 127.0f;
                    sendParam->setValueNotifyingHost (norm);
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

    const auto numSamples = buffer.getNumSamples();
    const auto numChannels = juce::jmin (buffer.getNumChannels(), dryBuffer.getNumChannels());
    const auto gatePreSoft = snap.gatePreSoft;

    for (int channel = 0; channel < numChannels; ++channel)
        dryBuffer.copyFrom (channel, 0, buffer, channel, 0, numSamples);

    for (int sample = 0; sample < numSamples; ++sample)
    {
        const auto inputGain = smoothedBank.getNextInputGainLinear();
        const auto sizeNorm = smoothedBank.getNextSizeNorm();
        const auto distnBlend = smoothedBank.getNextDistnBlend();
        const auto wetGain = smoothedBank.getNextLevelWetGain();
        const auto sendGain = smoothedBank.getNextSendGain();
        const auto thresholdNorm = smoothedBank.getNextInputThresholdNorm();
        const auto bypassWet = smoothedBank.getNextBypassWetMix();
        const auto outputGain = smoothedBank.getNextOutputGainLinear();

        (void) smoothedBank.getNextLevelDryGain();
        const auto darkModeMix = smoothedBank.getNextDarkModeTarget();
        const auto authenticColor = smoothedBank.getNextAuthenticColorTarget() > 0.5f;

        const auto rt60 = ParameterCurves::sizeToRT60 (sizeNorm);
        const auto thresholdDb = ParameterCurves::inputThresholdDb (thresholdNorm);
        const auto dryMix = 1.0f - bypassWet;

        float monoSum = 0.0f;

        for (int channel = 0; channel < numChannels; ++channel)
            monoSum += dryBuffer.getReadPointer (channel)[sample];

        monoSum /= static_cast<float> (numChannels);
        const auto monoIn = inputStage.processSample (monoSum, inputGain);
        const auto env = chain.getEnvelope().process (std::abs (monoIn));
        const auto wet = chain.processSample (monoIn, env, rt60, darkModeMix, authenticColor,
                                              distnBlend, sendGain, gatePreSoft, thresholdDb);

        for (int channel = 0; channel < numChannels; ++channel)
        {
            const auto dryTap = dryBuffer.getReadPointer (channel)[sample];
            const auto mixed = ParallelWetMixer::mix (dryTap, wet, wetGain);
            const auto preOutput = dryTap * dryMix + mixed * bypassWet;
            buffer.getWritePointer (channel)[sample] = OutputStage::processSample (preOutput, outputGain);
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
