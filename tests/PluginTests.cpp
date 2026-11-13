#include "Plugin.h"
#include <iostream>
#include <cstdlib>
void require(bool b,const char* text) { if(!b) { std::cerr<<"FAIL: "<<text<<'\n'; std::exit(1); } }
int main(int argc,char** argv) {
 juce::ScopedJuceInitialiser_GUI gui;
 VocalBalance processor;
 auto set=[&](const char* id,float v){auto* p=processor.state.getParameter(id);p->setValueNotifyingHost(p->convertTo0to1(v));};
 set("target",-18); set("music",4); set("idle",-2); set("mode",2); set("rider",3);
 juce::MemoryBlock saved; processor.getStateInformation(saved);
 VocalBalance restored; restored.setStateInformation(saved.getData(),(int)saved.getSize());
 require(std::abs(*restored.state.getRawParameterValue("target")+18)<0.01f,"target state restoration"); require(std::abs(*restored.state.getRawParameterValue("music")-4)<0.01f,"sidechain state restoration"); require(std::abs(*restored.state.getRawParameterValue("mode")-2)<0.01f,"mode restoration");
 restored.prepareToPlay(48000,256); juce::AudioBuffer<float> audio(2,256); juce::MidiBuffer midi;
 for(int k=0;k<600;++k) { for(int c=0;c<2;++c) for(int i=0;i<256;++i) audio.setSample(c,i,0.05f); restored.processBlock(audio,midi); }
 require(std::abs(restored.correction.load()-3)<0.03f,"Read mode follows rider automation");
 auto layout=restored.getBusesLayout(); layout.inputBuses.set(0,juce::AudioChannelSet::mono()); layout.outputBuses.set(0,juce::AudioChannelSet::mono()); require(restored.setBusesLayout(layout),"mono bus support");
 layout.inputBuses.set(0,juce::AudioChannelSet::stereo()); layout.outputBuses.set(0,juce::AudioChannelSet::stereo()); layout.inputBuses.set(1,juce::AudioChannelSet::stereo()); require(restored.setBusesLayout(layout),"stereo sidechain support");
 audio.setSize(4,256); set("mode",0); restored.releaseResources(); restored.prepareToPlay(48000,256); audio.clear(); restored.processBlock(audio,midi);
 std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor()); require(editor && editor->getWidth()==420 && editor->getHeight()==740,"editor creation and geometry");

 auto* target=restored.state.getParameter("target");
 restored.state.copyState(); restored.history.clearUndoHistory(); restored.history.beginNewTransaction("Target");
 target->setValueNotifyingHost(target->convertTo0to1(-12)); restored.state.copyState();
 require(restored.history.canUndo(),"undo history records parameter edits"); restored.history.undo();
 require(std::abs(*restored.state.getRawParameterValue("target")+18)<0.01f,"undo restores target"); restored.history.redo();
 require(std::abs(*restored.state.getRawParameterValue("target")+12)<0.01f,"redo restores edit");
 juce::TextButton* compare=nullptr;
 for(auto* child:editor->getChildren()) if(auto* button=dynamic_cast<juce::TextButton*>(child)) if(button->getButtonText()=="A / B") compare=button;
 require(compare!=nullptr,"A/B toolbar control"); compare->onClick();
 target->setValueNotifyingHost(target->convertTo0to1(-9)); restored.state.copyState(); compare->onClick();
 require(std::abs(*restored.state.getRawParameterValue("target")+12)<0.01f,"A/B restores independent snapshot");
 for(auto* child:editor->getChildren()) if(auto* button=dynamic_cast<juce::TextButton*>(child)) if(button->getName()=="attack-switch") {
  const float old=restored.state.getRawParameterValue("response")->load();button->onClick();
  require(std::abs(restored.state.getRawParameterValue("response")->load()-old)>0.5f,"Fast/Slow switch updates attack");
 }
 if(argc>1) { auto image=editor->createComponentSnapshot(editor->getLocalBounds()); juce::File output=juce::File::getCurrentWorkingDirectory().getChildFile(argv[1]); auto stream=output.createOutputStream(); require(stream!=nullptr,"screenshot output"); require(stream->setPosition(0),"screenshot seek"); require(stream->truncate().wasOk(),"screenshot truncate"); juce::PNGImageFormat png; require(png.writeImageToStream(image,*stream),"render editor PNG"); }

 auto* automation=restored.state.getParameter("mode");
 automation->setValueNotifyingHost(automation->convertTo0to1(1));
 restored.correction.store(5);
 const auto deadline=juce::Time::getMillisecondCounter()+1500;
 do {juce::Thread::sleep(10);juce::Timer::callPendingTimersSynchronously();}
 while(std::abs(*restored.state.getRawParameterValue("rider")-5)>=0.11f && juce::Time::getMillisecondCounter()<deadline);
 require(std::abs(*restored.state.getRawParameterValue("rider")-5)<0.11f,"Write mode publishes movement");
 automation->setValueNotifyingHost(automation->convertTo0to1(0));
 restored.correction.store(-5); juce::Thread::sleep(80); juce::Timer::callPendingTimersSynchronously();
 require(std::abs(*restored.state.getRawParameterValue("rider")-5)<0.11f,"Off mode leaves host Rider unchanged");
 restored.releaseResources(); std::cout<<"PASS: parameter state, Read/Write/Off modes, mono/stereo/sidechain, editor rendering, undo/redo, A/B\n";
}
