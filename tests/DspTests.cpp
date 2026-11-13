#include "Rider.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>
#include <new>
static bool audioCallback=false;
void* operator new(std::size_t size) { if(audioCallback) std::abort(); if(void* p=std::malloc(size?size:1)) return p; throw std::bad_alloc(); }
void* operator new[](std::size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { std::free(p); }
void operator delete(void* p,std::size_t) noexcept { std::free(p); }
void operator delete[](void* p,std::size_t) noexcept { std::free(p); }
void check(bool ok,const char* message) { if(!ok) { std::cerr<<"FAIL: "<<message<<'\n'; std::exit(1); } }
int main() {
 int scenarios=0;
 for(double rate:{44100.,48000.,96000.}) for(int block:{1,64,511,1024}) {
  vb::Rider rider; vb::Settings p; rider.prepare(rate);
  std::vector<float> left((size_t)block),right((size_t)block); float* ptr[]={left.data(),right.data()};
  auto run=[&](float amplitude,double seconds) {
   double phase=0; float peak=0;
   int remaining=(int)(rate*seconds);
   while(remaining>0) {
    const int n=std::min(block,remaining);
    for(int i=0;i<n;++i) { left[(size_t)i]=right[(size_t)i]=amplitude*(float)std::sin(phase); phase+=6.283185307179586*220/rate; }
    audioCallback=true; rider.process(ptr,2,n,p); audioCallback=false;
    for(int i=0;i<n;++i) { check(std::isfinite(left[(size_t)i]),"finite output"); check(left[(size_t)i]==right[(size_t)i],"stereo link"); peak=std::max(peak,std::abs(left[(size_t)i])); }
    check(rider.gain()>=p.minimum-0.01f && rider.gain()<=p.maximum+0.01f,"gain bounds"); remaining-=n;
   }
   return peak;
  };
  check(run(0,0.1)==0,"silence remains silent"); check(rider.gain()==0,"no gain ramp on silence");
  run(0.01f,3); check(rider.gain()>11.5f,"quiet signal reaches upper limit");
  run(0,3); check(std::abs(rider.gain())<0.1f,"idle returns to selected gain");
  run(0.8f,4); check(rider.gain()<-11.5f,"loud signal reaches lower limit");
  run(0.1f,4); check(std::abs(rider.level()+rider.gain()-p.target)<0.4f,"target convergence");
  p.bypass=true; run(0.1f,0.3); left[0]=0.123f; right[0]=-0.123f; rider.process(ptr,2,1,p); check(std::abs(left[0]-0.123f)<0.00001f,"bypass unity");
  left[0]=std::numeric_limits<float>::quiet_NaN(); right[0]=std::numeric_limits<float>::infinity(); rider.process(ptr,2,1,p); check(std::isfinite(left[0]) && std::isfinite(right[0]),"nonfinite input containment");
  p.bypass=false; p.overrideGain=true; p.manualGain=-4; run(0.1f,3); check(std::abs(rider.gain()+4)<0.1f,"manual gain override");
  p.overrideGain=false; p.idle=2; run(0,4); check(std::abs(rider.gain()-2)<0.1f,"nonzero idle");
  // Abrupt transients and response to block-to-block parameter changes.
  p.idle=0; p.output=6; p.response=10; run(0.9f,0.02); p.output=-6; run(0.001f,0.02);
  ++scenarios;
 }
 vb::Settings sideSettings; sideSettings.minimum=-12; sideSettings.maximum=12; sideSettings.musicInfluence=1;
 auto sideGain=[&](float musicLevel) { vb::Rider r; r.prepare(48000); std::array<float,64> voice{},music{}; float* v[]={voice.data()}; const float* m[]={music.data()}; for(int k=0;k<4000;++k) { voice.fill(0.063f); music.fill(musicLevel); r.process(v,1,64,sideSettings,m,1); } return r.gain(); };
 check(sideGain(0.25f)>sideGain(0.063f)+8,"sidechain follows accompaniment level");
 std::cout<<"PASS: "<<scenarios<<" sample-rate/block-size scenarios; silence, gain bounds, stereo link, idle, convergence, bypass, nonfinite input\n";
}
