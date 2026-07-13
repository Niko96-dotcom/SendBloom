#pragma once

#include <atomic>
#include <vector>
#include <juce_audio_processors/juce_audio_processors.h>
#include "ParameterSnapshot.h"
#include "SmoothedParameterBank.h"
#include "GatedBloomChain.h"
#include "InputStage.h"
#include "OutputStage.h"
#include "PressureController.h"

namespace sendbloom
{

class PluginProcessor : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener
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
    bool isCurrentProgramCustom() const noexcept { return currentProgramCustom_.load(); }
    juce::String getCurrentProgramDisplayName() const;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }
    const juce::AudioProcessorValueTreeState& getAPVTS() const noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorParameter* getBypassParameter() const override;

    bool isClipHoldActive() const noexcept { return clipHoldFlag.load(); }

    juce::AudioProcessorValueTreeState apvts;

    SmoothedParameterBank smoothedBank;
    PressureController pressureController;
    juce::AudioBuffer<float> dryBuffer;
    GatedBloomChain chain;
    InputStage inputStage;

    /** ADR-V1-05 control-rate latch quantum (samples). */
    static constexpr int kControlQuantum = 128;

private:
    std::atomic<bool> clipHoldFlag { false };
    std::atomic<int> currentProgramIndex_ { 0 };
    std::atomic<bool> currentProgramCustom_ { false };
    std::atomic<bool> programStateUpdateInProgress_ { false };
    int preparedMaxBlock_ { 0 };
    std::vector<float> monoScratch_;
    std::vector<float> envelopeScratch_;
    std::vector<float> wetScratch_;
    std::vector<float> wetGainScratch_;
    std::vector<float> sendGainScratch_;
    std::vector<float> distnScratch_;
    std::vector<float> thresholdLinearScratch_;
    std::vector<float> bypassWetScratch_;
    std::vector<float> outputGainScratch_;
    void applyPressureMidiAtSample (const juce::MidiBuffer& midiMessages,
                                    int samplePosition) noexcept;
    int findNextPressureMidiSampleAfter (const juce::MidiBuffer& midiMessages,
                                         int afterSample,
                                         int numSamples) const noexcept;
    void processSpan (juce::AudioBuffer<float>& buffer,
                      int offset,
                      int span,
                      const ParameterSnapshot& snap) noexcept;
    void parameterChanged (const juce::String& parameterID, float newValue) override;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};

} // namespace sendbloom
