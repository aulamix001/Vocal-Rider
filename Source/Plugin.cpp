#include "Plugin.h"
#include "Editor.h"
juce::AudioProcessorValueTreeState::ParameterLayout VocalBalance::parameters() {
 juce::AudioProcessorValueTreeState::ParameterLayout p;
 auto add=[&](const char* id,const char* name,float lo,float hi,float initial){ p.add(std::make_unique<juce::AudioParameterFloat>(juce::ParameterID{id,1},name,juce::NormalisableRange<float>{lo,hi,0.1f},initial)); };
 add("target","Target (dBFS)",-42,0,-21); add("minimum","Minimum gain (dB)",-12,11,-6); add("maximum","Maximum gain (dB)",-11,12,6); add("gate","Vocal sensitivity (dB)",-12,12,0); p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"response",1},"Attack",juce::StringArray{"Fast","Slow"},1)); add("output","Output (dB)",-12,12,0); add("music","Music sensitivity (dB)",-12,12,0); add("idle","Idle (dB)",-12,12,0);
 add("rider","Rider (dB)",-12,12,0); p.add(std::make_unique<juce::AudioParameterChoice>(juce::ParameterID{"mode",1},"Automation",juce::StringArray{"Off","Write","Read"},0));
 p.add(std::make_unique<juce::AudioParameterBool>(juce::ParameterID{"bypass",1},"Bypass",false)); return p;
}
VocalBalance::VocalBalance(): AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true).withInput("Music sidechain",juce::AudioChannelSet::stereo(),false)),state(*this,&history,"VocalBalance",parameters()) {
 const char* ids[]={"target","minimum","maximum","gate","response","output","bypass","music","idle","rider","mode"}; for(size_t i=0;i<11;++i) values[i]=state.getRawParameterValue(ids[i]); startTimerHz(30);
}
void VocalBalance::prepareToPlay(double rate,int) { rider.prepare(rate); }
bool VocalBalance::isBusesLayoutSupported(const BusesLayout& b) const { return b.getMainInputChannelSet()==b.getMainOutputChannelSet() && (b.getMainOutputChannelSet()==juce::AudioChannelSet::mono() || b.getMainOutputChannelSet()==juce::AudioChannelSet::stereo()); }
void VocalBalance::processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&) {
 juce::ScopedNoDenormals noDenormals;
 vb::Settings p; p.target=values[0]->load(); p.minimum=values[1]->load(); p.maximum=values[2]->load(); p.gate=-55-values[3]->load(); p.response=values[4]->load()<0.5f?100.0f:600.0f; p.output=values[5]->load(); p.bypass=values[6]->load()>0.5f; p.musicInfluence=std::pow(10.0f,values[7]->load()/20.0f); p.idle=values[8]->load(); p.manualGain=values[9]->load(); p.overrideGain=manualTouch.load() || values[10]->load()>1.5f;
 auto main=getBusBuffer(b,true,0);
 auto music=getBusBuffer(b,true,1);
 float musicPeak=0; for(int c=0;c<music.getNumChannels();++c) musicPeak=std::max(musicPeak,music.getMagnitude(c,0,music.getNumSamples()));
 musicActive.store(musicPeak>0.001f,std::memory_order_relaxed);
 rider.process(main.getArrayOfWritePointers(),main.getNumChannels(),main.getNumSamples(),p,music.getArrayOfReadPointers(),music.getNumChannels());
 inputLevel.store(rider.level(),std::memory_order_relaxed); correction.store(rider.gain(),std::memory_order_relaxed);
 float peak=0; for(int c=0;c<main.getNumChannels();++c) peak=std::max(peak,main.getMagnitude(c,0,main.getNumSamples()));
 outputLevel.store(juce::Decibels::gainToDecibels(peak,-120.0f),std::memory_order_relaxed);
}
const juce::String VocalBalance::getProgramName(int n) { return n==1?"Gentle":n==2?"Firm":"Default"; }
void VocalBalance::setCurrentProgram(int n) {
 currentProgram=juce::jlimit(0,2,n);
 const float presets[3][6]={{-21,-6,6,0,1,0},{-21,-3,3,0,1,0},{-21,-9,9,3,0,0}};
 const char* ids[]={"target","minimum","maximum","gate","response","output"};
 for(int i=0;i<6;++i) { auto* p=state.getParameter(ids[i]); p->beginChangeGesture(); p->setValueNotifyingHost(p->convertTo0to1(presets[currentProgram][i])); p->endChangeGesture(); }
}
void VocalBalance::getStateInformation(juce::MemoryBlock& data) { auto tree=state.copyState(); tree.setProperty("program",currentProgram,nullptr); if(auto xml=tree.createXml()) copyXmlToBinary(*xml,data); }
void VocalBalance::setStateInformation(const void* data,int size) { if(auto xml=getXmlFromBinary(data,size)) if(xml->hasTagName(state.state.getType())) { auto tree=juce::ValueTree::fromXml(*xml); currentProgram=juce::jlimit(0,2,(int)tree.getProperty("program",0)); state.replaceState(tree); } }
juce::AudioProcessorEditor* VocalBalance::createEditor() { return new BalanceEditor(*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new VocalBalance; }

void VocalBalance::timerCallback() {
 const int mode=(int)values[10]->load(); auto* param=state.getParameter("rider");
 if(mode==1 && !manualTouch.load()) { param->beginChangeGesture(); param->setValueNotifyingHost(param->convertTo0to1(correction.load())); param->endChangeGesture(); }
}
