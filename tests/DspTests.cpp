#include "Dsp/GainRider.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <new>
#include <vector>
static bool audioCallback=false;
void* operator new(std::size_t n){if(audioCallback)std::abort();if(void* p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{std::free(p);}void operator delete[](void* p)noexcept{std::free(p);}
void operator delete(void* p,std::size_t)noexcept{std::free(p);}void operator delete[](void* p,std::size_t)noexcept{std::free(p);}
void check(bool value,const char* message){if(!value){std::cerr<<"FAIL: "<<message<<'\n';std::exit(1);}}
int main(){
 int scenarios=0;
 for(double rate:{44100.,48000.,96000.})for(int size:{1,64,511,1024}){
  vocalbalance::GainRider rider;rider.prepare(rate);vocalbalance::Controls c;
  std::vector<float> left(static_cast<size_t>(size)),right(static_cast<size_t>(size));float* p[]={left.data(),right.data()};
  auto run=[&](float input,double seconds){int remaining=static_cast<int>(rate*seconds);while(remaining>0){int n=std::min(size,remaining);std::fill(left.begin(),left.end(),input);std::fill(right.begin(),right.end(),input);audioCallback=true;rider.process(p,2,n,c);audioCallback=false;for(int i=0;i<n;++i){check(std::isfinite(left[static_cast<size_t>(i)]),"finite output");check(left[static_cast<size_t>(i)]==right[static_cast<size_t>(i)],"linked stereo");}remaining-=n;}return left[0];};
  check(run(0,.2)==0,"silence stays silent");check(!rider.isActive(),"silence is inactive");
  run(.005f,4);check(rider.gainDb()>5.9f,"quiet voice raises gain to upper bound");
  run(.8f,4);check(rider.gainDb()<-5.9f,"loud voice lowers gain to lower bound");
  run(.1f,4);check(std::abs(rider.inputDb()+rider.gainDb()-c.target)<.1f,"target convergence");
  c.idle=2;run(0,6);check(std::abs(rider.gainDb()-2)<.05f,"returns to selected Idle");
  c.manual=true;c.rider=-4;run(.1f,.5);check(std::abs(rider.gainDb()+4)<.01f,"manual override");
  c.bypass=true;check(std::abs(run(.1f,.5)-.1f)<.00001f,"bypass is unity");
  left[0]=std::numeric_limits<float>::quiet_NaN();right[0]=std::numeric_limits<float>::infinity();rider.process(p,2,1,c);check(std::isfinite(left[0])&&std::isfinite(right[0]),"nonfinite containment");
  left[0]=right[0]=std::numeric_limits<float>::max();c.bypass=false;c.output=12;rider.process(p,2,1,c);check(std::isfinite(left[0]),"finite maximum input containment");
  ++scenarios;
 }
 auto gainWithMusic=[](float backing){vocalbalance::GainRider rider;rider.prepare(48000);vocalbalance::Controls c;std::array<float,256> voice{},music{};float* p[]={voice.data()};const float* side[]={music.data()};for(int k=0;k<800;++k){voice.fill(.1f);music.fill(backing);rider.process(p,1,256,c,side,1);}return rider.gainDb();};
 check(gainWithMusic(.25f)>gainWithMusic(.063f)+9,"musical sidechain changes vocal target");
 std::cout<<"PASS: "<<scenarios<<" rate/block-size combinations; silence, limits, level changes, idle, manual, bypass, finite samples, sidechain; no new in DSP\n";
}
