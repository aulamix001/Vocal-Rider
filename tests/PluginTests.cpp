#include "PluginProcessor.h"
#include <cstdlib>
#include <iostream>
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(int argc,char**argv){
 juce::ScopedJuceInitialiser_GUI gui;
 VocalBalanceAudioProcessor first;
 auto set=[&](VocalBalanceAudioProcessor& p,const char* id,float value){auto* param=p.state.getParameter(id);check(param!=nullptr,"parameter exists");param->setValueNotifyingHost(param->convertTo0to1(value));};
 set(first,"target",-18);set(first,"mode",2);set(first,"rider",3);set(first,"music",4);
 juce::MemoryBlock state;first.getStateInformation(state);
 VocalBalanceAudioProcessor p;p.setStateInformation(state.getData(),static_cast<int>(state.getSize()));
 check(std::abs(p.state.getRawParameterValue("target")->load()+18)<.01f,"state target restored");
 check(std::abs(p.state.getRawParameterValue("music")->load()-4)<.01f,"state sidechain restored");
 p.prepareToPlay(48000,256);juce::AudioBuffer<float> audio(2,256);juce::MidiBuffer midi;
 for(int k=0;k<300;++k){for(int c=0;c<2;++c)std::fill_n(audio.getWritePointer(c),256,.05f);p.processBlock(audio,midi);}
 check(std::abs(p.gainDb.load()-3)<.02f,"Read follows automation");
 auto layout=p.getBusesLayout();layout.inputBuses.set(0,juce::AudioChannelSet::mono());layout.outputBuses.set(0,juce::AudioChannelSet::mono());check(p.setBusesLayout(layout),"mono layout accepted");
 layout.inputBuses.set(0,juce::AudioChannelSet::stereo());layout.outputBuses.set(0,juce::AudioChannelSet::stereo());layout.inputBuses.set(1,juce::AudioChannelSet::stereo());check(p.setBusesLayout(layout),"stereo plus music accepted");
 auto invalid=layout;invalid.outputBuses.set(0,juce::AudioChannelSet::mono());check(!p.isBusesLayoutSupported(invalid),"mismatched buses rejected");
 std::unique_ptr<juce::AudioProcessorEditor> editor(p.createEditor());check(editor&&editor->getWidth()==420&&editor->getHeight()==740,"editor geometry");
 p.state.copyState();p.history.clearUndoHistory();p.history.beginNewTransaction("Target");set(p,"target",-12);p.state.copyState();check(p.history.canUndo(),"undo available");p.history.undo();check(std::abs(p.state.getRawParameterValue("target")->load()+18)<.01f,"undo restores parameter");p.history.redo();check(std::abs(p.state.getRawParameterValue("target")->load()+12)<.01f,"redo restores edit");
 set(p,"mode",1);p.gainDb.store(5);auto deadline=juce::Time::getMillisecondCounter()+2000;
 do{juce::Thread::sleep(10);juce::Timer::callPendingTimersSynchronously();}while(std::abs(p.state.getRawParameterValue("rider")->load()-5)>.1f&&juce::Time::getMillisecondCounter()<deadline);
 check(std::abs(p.state.getRawParameterValue("rider")->load()-5)<.1f,"Write publishes movement");
 set(p,"mode",0);p.gainDb.store(-5);juce::Thread::sleep(80);juce::Timer::callPendingTimersSynchronously();check(std::abs(p.state.getRawParameterValue("rider")->load()-5)<.1f,"Off ignores host Rider changes");
 p.setCurrentProgram(1);check(p.getCurrentProgram()==1&&std::abs(p.state.getRawParameterValue("lower")->load()+3)<.01f,"factory preset");
 if(argc>1){auto image=editor->createComponentSnapshot(editor->getLocalBounds());auto stream=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]).createOutputStream();check(stream!=nullptr,"image output");stream->setPosition(0);stream->truncate();juce::PNGImageFormat png;check(png.writeImageToStream(image,*stream),"editor renders");}
 std::cout<<"PASS: state, bus layouts, editor, automation modes, undo/redo and presets\n";
}
