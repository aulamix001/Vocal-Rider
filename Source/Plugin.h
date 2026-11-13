#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "Rider.h"
class VocalBalance final : public juce::AudioProcessor, private juce::Timer {
public:
 VocalBalance();
 ~VocalBalance() override { stopTimer(); }
 using juce::AudioProcessor::processBlock;
 void prepareToPlay(double,int) override;
 void releaseResources() override {}
 bool isBusesLayoutSupported(const BusesLayout&) const override;
 void processBlock(juce::AudioBuffer<float>&,juce::MidiBuffer&) override;
 juce::AudioProcessorEditor* createEditor() override;
 bool hasEditor() const override { return true; }
 const juce::String getName() const override { return "Vocal Balance"; }
 bool acceptsMidi() const override { return false; }
 bool producesMidi() const override { return false; }
 double getTailLengthSeconds() const override { return 0; }
 int getNumPrograms() override { return 3; }
 int getCurrentProgram() override { return currentProgram; }
 void setCurrentProgram(int) override;
 const juce::String getProgramName(int) override;
 void changeProgramName(int,const juce::String&) override {}
 void getStateInformation(juce::MemoryBlock&) override;
 void setStateInformation(const void*,int) override;
 juce::UndoManager history;
 juce::AudioProcessorValueTreeState state;
 std::atomic<bool> manualTouch{false}, musicActive{false};
 std::atomic<float> inputLevel{-120}, correction{0}, outputLevel{-120};
private:
 void timerCallback() override;

 static juce::AudioProcessorValueTreeState::ParameterLayout parameters();
 vb::Rider rider;
 std::array<std::atomic<float>*,11> values{};
 int currentProgram=0;
};
