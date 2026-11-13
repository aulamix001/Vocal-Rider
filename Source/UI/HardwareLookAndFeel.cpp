#include "HardwareLookAndFeel.h"
namespace { const juce::Colour dark{0xff20282c}, ivory{0xffeeecdf}; }
void HardwareLookAndFeel::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool down) {
 auto r=b.getLocalBounds().toFloat().reduced(.5f);
 if(b.getName()=="attack-switch") {
  g.setColour(juce::Colour{0xff080d0e});g.fillRect(3,2,12,b.getHeight()-4);g.setColour(juce::Colour{0xff737d7e});g.drawRect(3,2,12,b.getHeight()-4);
  int y=b.getButtonText()=="Fast"?4:b.getHeight()-15;g.setGradientFill(juce::ColourGradient(juce::Colour{0xff89908a},3.f,(float)y,juce::Colour{0xff232b2e},3.f,(float)y+11,false));g.fillRect(4,y,10,11);return;
 }
 auto c=b.getToggleState()?b.findColour(juce::TextButton::buttonOnColourId):colour;
 if(hover) c=c.brighter(.1f);
 if(down)c=c.darker(.2f);
 g.setColour(juce::Colour{0xff101514});g.fillRect(r);
 g.setGradientFill(juce::ColourGradient(c.brighter(.3f),r.getTopLeft(),c.darker(.35f),r.getBottomLeft(),false));g.fillRect(r.reduced(1));
 g.setColour(juce::Colours::white.withAlpha(down?.15f:.4f));g.drawLine(r.getX()+1,r.getY()+1,r.getRight()-1,r.getY()+1);
 g.setColour(juce::Colours::black.withAlpha(.7f));g.drawLine(r.getX(),r.getBottom()-1,r.getRight(),r.getBottom()-1);
}
void HardwareLookAndFeel::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool hover,bool down) {
 if(b.getName()=="attack-switch") {g.setFont(11);g.setColour(ivory.withAlpha(b.getButtonText()=="Fast"?1.f:.5f));g.drawText("Fast",20,1,40,17,juce::Justification::centredLeft);g.setColour(ivory.withAlpha(b.getButtonText()=="Slow"?1.f:.5f));g.drawText("Slow",20,19,40,17,juce::Justification::centredLeft);return;}
 juce::LookAndFeel_V4::drawButtonText(g,b,hover,down);
}
void HardwareLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& s) {
 const bool vertical=style==juce::Slider::LinearVertical, rider=s.getName()=="rider";
 if(s.getName()=="idle") { juce::Path arrow; arrow.addTriangle((float)x+3,pos-5,(float)x+3,pos+5,(float)x+11,pos);g.setColour(juce::Colour{0xff183445});g.fillPath(arrow);return; }
 if(vertical) {
  g.setColour(juce::Colours::white.withAlpha(.42f));g.fillRect(x+w/2-7,y,14,h);
  g.setColour(juce::Colour{0xff686a68});g.fillRect(x+w/2-5,y,10,h);
  g.setColour(juce::Colour{0xff111514});g.fillRect(x+w/2-2,y,4,h);
 } else {
  g.setColour(juce::Colour{0xff090b0b});g.fillRect(x-2,y+h/2-6,w+4,12);
  g.setColour(juce::Colour{0xff625e23});g.fillRect(x,y+h/2-3,w,6);
  float level=(float)s.getProperties().getWithDefault("level",-120.f);
  g.setGradientFill(juce::ColourGradient(juce::Colour{0xffffed86},0,(float)(y+h/2-3),juce::Colour{0xffa49c1f},0,(float)(y+h/2+3),false));
  g.fillRect((float)x,(float)(y+h/2-3),juce::jlimit(0.f,1.f,(level+42)/42)*(float)w,6.f);
 }
 juce::Rectangle<float> cap=vertical?juce::Rectangle<float>((float)(x+w/2)-(rider?28.f:20.f),pos-(rider?32.f:10.f),rider?56.f:40.f,rider?64.f:20.f):juce::Rectangle<float>(pos-10.f,(float)(y+h/2)-10,20,20);
 for(int spread=7;spread>=1;--spread) {g.setColour(juce::Colours::black.withAlpha(.025f));g.fillRoundedRectangle(cap.expanded((float)spread).translated(3,5),3);}
 auto light=rider?juce::Colour{0xfff1f0dc}:juce::Colour{0xff73797a};
 auto shade=rider?juce::Colour{0xff96998b}:juce::Colour{0xff151b1d};
 juce::ColourGradient metal(light,cap.getTopLeft(),shade,cap.getBottomLeft(),false);
 metal.addColour(.10,shade.brighter(.35f));metal.addColour(.22,light);metal.addColour(.50,rider?juce::Colour{0xffd8d9c6}:juce::Colour{0xff323b3e});metal.addColour(.82,light.darker(.25f));metal.addColour(.93,shade);
 g.setGradientFill(metal);g.fillRoundedRectangle(cap,2);
 g.setColour(juce::Colours::black.withAlpha(.8f));g.drawRoundedRectangle(cap,2,1);
 g.setColour(juce::Colours::white.withAlpha(.6f));g.drawHorizontalLine((int)cap.getY()+1,cap.getX()+2,cap.getRight()-2);
 if(rider) for(int offset:{-23,-17,-11,11,17,23}) {g.setColour(juce::Colour{0xff6f756d});g.drawHorizontalLine((int)cap.getCentreY()+offset,cap.getX()+3,cap.getRight()-3);g.setColour(juce::Colours::white.withAlpha(.8f));g.drawHorizontalLine((int)cap.getCentreY()+offset+1,cap.getX()+3,cap.getRight()-3);}
 g.setColour(rider?juce::Colour{0xffb44632}:juce::Colour{0xffaab3b2});g.drawHorizontalLine((int)cap.getCentreY(),cap.getX()+2,cap.getRight()-2);
}
void HardwareLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&) {
 auto area=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(12);float d=std::min(area.getWidth(),area.getHeight());auto r=area.withSizeKeepingCentre(d,d);auto c=r.getCentre();
 for(int spread=5;spread>=1;--spread) {g.setColour(juce::Colours::black.withAlpha(.08f));g.fillEllipse(r.expanded((float)spread).translated(2,4));}
 g.setColour(juce::Colour{0xffadb4b2});for(int i=0;i<=10;++i) {float a=start+(end-start)*(float)i/10;juce::Point<float> u{std::sin(a),-std::cos(a)};g.drawLine({c+u*(d/2+4),c+u*(d/2+8)},i%5==0?1.4f:.8f);}
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xffd4d8ce},r.getTopLeft(),juce::Colour{0xff343c3d},r.getBottomRight(),false));g.fillEllipse(r);
 auto face=r.reduced(3);g.setColour(juce::Colour{0xff101618});g.fillEllipse(face);
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xff596267},face.getTopLeft(),juce::Colour{0xff1e282d},face.getBottomRight(),false));g.fillEllipse(face.reduced(2));
 g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(face.reduced(4),.7f);
 float a=start+(end-start)*position;juce::Point<float> u{std::sin(a),-std::cos(a)};g.setColour(ivory);g.drawLine({c+u*(d*.12f),c+u*(d/2-6)},2.5f);
}

