// Minimal Linux host: load the actual bundle module and process audio via VST3 ABI.
#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include <dlfcn.h>
#include <array>
#include <cmath>
#include <cstring>
#include <iostream>
#include <cstdlib>
using namespace Steinberg;
using namespace Steinberg::Vst;
void require(bool ok,const char* message) { if(!ok) {std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);} }
int main(int argc,char** argv) {
 require(argc==2,"provide path to VST3 .so");
 void* lib=dlopen(argv[1],RTLD_NOW|RTLD_LOCAL); if(!lib) {std::cerr<<dlerror()<<'\n';return 1;}
 auto enter=reinterpret_cast<bool(*)(void*)>(dlsym(lib,"ModuleEntry"));
 auto leave=reinterpret_cast<bool(*)()>(dlsym(lib,"ModuleExit"));
 require(enter && enter(lib),"module entry");
 auto factoryFn=reinterpret_cast<IPluginFactory*(*)()>(dlsym(lib,"GetPluginFactory"));require(factoryFn!=nullptr,"factory export");
 IPluginFactory* factory=factoryFn(); require(factory!=nullptr,"factory instance");
 PClassInfo info{}; bool found=false;
 for(int i=0;i<factory->countClasses();++i) {factory->getClassInfo(i,&info); if(std::strcmp(info.category,kVstAudioEffectClass)==0) {found=true;break;}}
 require(found,"audio effect class");
 IComponent* component=nullptr; require(factory->createInstance(info.cid,IComponent::iid.toTUID(),reinterpret_cast<void**>(&component))==kResultOk && component,"component creation");
 require(component->initialize(nullptr)==kResultOk,"component initialization");
 IAudioProcessor* processor=nullptr;require(component->queryInterface(IAudioProcessor::iid.toTUID(),reinterpret_cast<void**>(&processor))==kResultOk,"audio processor interface");
 ProcessSetup setup{};setup.processMode=kRealtime;setup.symbolicSampleSize=kSample32;setup.maxSamplesPerBlock=256;setup.sampleRate=48000;
 require(processor->setupProcessing(setup)==kResultOk,"processing setup");
 component->activateBus(kAudio,kInput,0,true);component->activateBus(kAudio,kOutput,0,true);
 require(component->setActive(true)==kResultOk,"activate");require(processor->setProcessing(true)==kResultOk,"start processing");
 std::array<float,256> inL{},inR{},outL{},outR{};float* inputs[]={inL.data(),inR.data()};float* outputs[]={outL.data(),outR.data()};
 AudioBusBuffers in{},out{};in.numChannels=out.numChannels=2;in.channelBuffers32=inputs;out.channelBuffers32=outputs;
 ProcessData data{};data.processMode=kRealtime;data.symbolicSampleSize=kSample32;data.numSamples=256;data.numInputs=data.numOutputs=1;data.inputs=&in;data.outputs=&out;
 double energy=0;
 for(int block=0;block<500;++block) { for(int i=0;i<256;++i) inL[(size_t)i]=inR[(size_t)i]=0.03f*(float)std::sin(6.28318530718*220*(block*256+i)/48000);
  require(processor->process(data)==kResultOk,"process block");
  for(int i=0;i<256;++i) {require(std::isfinite(outL[(size_t)i]),"finite plugin output");require(outL[(size_t)i]==outR[(size_t)i],"linked plugin stereo");energy+=outL[(size_t)i]*outL[(size_t)i];}
 }
 require(energy>1,"non-silent processed audio");
 processor->setProcessing(false);component->setActive(false);processor->release();component->terminate();component->release();factory->release();if(leave) leave();dlclose(lib);
 std::cout<<"PASS: VST3 module factory, activation and 500 stereo audio blocks\n";
}
