#pragma once

#include <atomic>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterSnapshot.h"
#include "SmoothedParameterBank.h"
#include "GatedBloomChain.h"
#include "InputStage.h"
#include "OutputStage.h"

namespace sendbloom
{

class PluginProcessor : public juce::AudioProcessor
{
public:
    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorParameter* getBypassParameter() const override;

    bool isClipHoldActive() const noexcept { return clipHoldFlag.load(); }

    juce::AudioProcessorValueTreeState apvts;

    SmoothedParameterBank smoothedBank;
    juce::AudioBuffer<float> dryBuffer;
    GatedBloomChain chain;
    InputStage inputStage;

private:
    std::atomic<bool> clipHoldFlag { false };
    int currentProgramIndex { 0 };
    int preparedMaxBlock_ { 0 };
    std::vector<float> monoScratch_;
    std::vector<float> envelopeScratch_;
    std::vector<float> wetScratch_;
    std::vector<float> wetGainScratch_;
    std::vector<float> bypassWetScratch_;
    std::vector<float> outputGainScratch_;
    float lastAuthenticColorSmoothed_ { 0.0f };

    void updateReportedLatency (bool targetAuthenticOn) noexcept;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

} // namespace sendbloom
