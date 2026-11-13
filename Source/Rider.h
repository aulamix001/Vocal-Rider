#pragma once
#include <algorithm>
#include <cmath>
namespace vb {
struct Settings { float target=-24, minimum=-12, maximum=12, gate=-55, response=250, output=0, musicInfluence=0, idle=0; float manualGain=0; bool overrideGain=false; bool bypass=false; };
class Rider {
public:
 void prepare(double rate) noexcept { sr=std::max(8000.0,rate); energy=0; musicEnergy=0; gainDb=0; trimDb=0; wet=1; }
 float gain() const noexcept { return gainDb; }
 float level() const noexcept { return 10*std::log10(std::max(energy,1.0e-12f)); }
 void process(float* const* channels,int count,int samples,const Settings& p, const float* const* music=nullptr, int musicChannels=0) noexcept {
  const float detector=static_cast<float>(std::exp(-1.0/(sr*(p.response<=100?0.01:0.05))));
  const float movement=static_cast<float>(std::exp(-1.0/(sr*std::max(10.0f,p.response)*0.001)));
  const float smooth=static_cast<float>(std::exp(-1.0/(sr*0.01)));
  const float lo=std::min(p.minimum,p.maximum), hi=std::max(p.minimum,p.maximum);
  for(int i=0;i<samples;++i) {
   float power=0;
   for(int c=0;c<count;++c) { float x=channels[c][i]; if(!std::isfinite(x)) x=0; channels[c][i]=x; power+=x*x; }
   power/=static_cast<float>(std::max(1,count));
   energy=detector*energy+(1-detector)*power;
   // Return to the selected idle position below the activity threshold.
   float musicPower=0;
   for(int c=0;c<musicChannels;++c) { const float x=music[c][i]; if(std::isfinite(x)) musicPower+=x*x; }
   musicEnergy=detector*musicEnergy+(1-detector)*musicPower/static_cast<float>(std::max(1,musicChannels));
   const float musicDb=10*std::log10(std::max(musicEnergy,1.0e-12f));
   const float musicOffset=musicChannels>0 && musicDb>-60 ? std::clamp((musicDb+18)*p.musicInfluence,-12.0f,12.0f) : 0;
   const float desired=p.overrideGain ? std::clamp(p.manualGain,-12.0f,12.0f) : level()>p.gate ? std::clamp(p.target+musicOffset-level(),lo,hi) : std::clamp(p.idle,lo,hi);
   const float coefficient=p.overrideGain?smooth:movement;
   gainDb=coefficient*gainDb+(1-coefficient)*desired;
   trimDb=smooth*trimDb+(1-smooth)*p.output;
   wet=smooth*wet+(1-smooth)*(p.bypass?0.0f:1.0f);
   const float multiplier=1+wet*(std::pow(10.0f,(gainDb+trimDb)*0.05f)-1);
   for(int c=0;c<count;++c) channels[c][i]*=multiplier;
  }
 }
private:
 double sr=48000; float energy=0,musicEnergy=0,gainDb=0,trimDb=0,wet=1;
};
}
