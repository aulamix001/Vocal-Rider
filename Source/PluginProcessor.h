#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Dsp/GainRider.h"

class VocalBalanceAudioProcessor final : public juce::AudioProcessor, private juce::Timer {
public:
    VocalBalanceAudioProcessor();
    ~VocalBalanceAudioProcessor() override;
    using juce::AudioProcessor::processBlock;
    const juce::String getName() const override { return "Vocal Balance"; }
    void prepareToPlay(double, int) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported(const BusesLayout&) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    bool hasEditor() const override { return true; }
    juce::AudioProcessorEditor* createEditor() override;
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    double getTailLengthSeconds() const override { return 0; }
    int getNumPrograms() override { return 3; }
    int getCurrentProgram() override { return preset.load(); }
    void setCurrentProgram(int) override;
    const juce::String getProgramName(int) override;
    void changeProgramName(int, const juce::String&) override {}
    void getStateInformation(juce::MemoryBlock&) override;
    void setStateInformation(const void*, int) override;

    juce::UndoManager history;
    juce::AudioProcessorValueTreeState state;
    std::atomic<float> inputLevel{-120}, gainDb{0}, outputLevel{-120};
    std::atomic<bool> vocalActive{false}, musicActive{false}, manualRider{false};
    void startAutomationUpdates();
private:
    void timerCallback() override;
    static juce::AudioProcessorValueTreeState::ParameterLayout makeParameters();
    std::array<std::atomic<float>*, 11> parameters{};
    vocalbalance::GainRider rider;
    std::atomic<int> preset{0};
};
