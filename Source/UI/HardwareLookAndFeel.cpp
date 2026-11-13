#include "HardwareLookAndFeel.h"
namespace { const juce::Colour dark{0xff20282c}, ivory{0xffeeecdf}; }
juce::Slider::SliderLayout HardwareLookAndFeel::getSliderLayout(juce::Slider& slider) {
    auto layout=juce::LookAndFeel_V4::getSliderLayout(slider);
    if(slider.getSliderStyle()==juce::Slider::LinearVertical){
        const int margin=slider.getName()=="rider"?56:(slider.getName()=="output"?34:10);
        layout.sliderBounds=slider.getLocalBounds().reduced(0,margin);
    }
    return layout;
}
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
 if(b.getName()=="automation") {g.setFont(juce::FontOptions(10.5f,juce::Font::bold));g.setColour(b.getButtonText()=="OFF"?juce::Colour{0xff24251c}:ivory);g.drawText(b.getButtonText(),b.getLocalBounds(),juce::Justification::centred);return;}
 juce::LookAndFeel_V4::drawButtonText(g,b,hover,down);
}
void HardwareLookAndFeel::drawFaderTrack(juce::Graphics& g, float centre, float top, float bottom) {
    juce::Rectangle<float> recess{centre-8,top,16,bottom-top};
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xff78767c},recess.getTopLeft(),juce::Colour{0xffe0dde1},recess.getTopRight(),false));
    g.fillRect(recess);
    g.setColour(juce::Colour{0xff2b292d});g.fillRect(centre-3,top,6.f,bottom-top);
    g.setColour(juce::Colour{0xff08070a});g.fillRect(centre-1,top,3.f,bottom-top);
    g.setColour(juce::Colours::white.withAlpha(.42f));g.drawVerticalLine(static_cast<int>(centre+8),top,bottom);
}
void HardwareLookAndFeel::drawFaderCap(juce::Graphics& g, juce::Rectangle<float> cap, bool rider) {
    // Long cast shadow, contact shadow, machined bevels and individual raised ribs.
    for(int spread=17;spread>0;--spread){
        g.setColour(juce::Colours::black.withAlpha(.013f));
        g.fillRoundedRectangle(cap.expanded(spread*.5f,spread*.7f).translated(4,9+spread*.18f),3);
    }
    g.setColour(juce::Colours::black.withAlpha(.5f));g.fillRoundedRectangle(cap.translated(1,3).expanded(1),2);
    juce::ColourGradient body(rider?juce::Colour{0xffefede3}:juce::Colour{0xff8d8b8e},cap.getTopLeft(),rider?juce::Colour{0xff8d8c83}:juce::Colour{0xff18171a},cap.getBottomLeft(),false);
    body.addColour(.06,juce::Colour{0xffeeeae4});body.addColour(.12,rider?juce::Colour{0xffb4b3a8}:juce::Colour{0xff57565a});
    body.addColour(.28,rider?juce::Colour{0xffdeddd1}:juce::Colour{0xff3b393e});
    body.addColour(.68,rider?juce::Colour{0xffc2c3b6}:juce::Colour{0xff252328});
    body.addColour(.87,rider?juce::Colour{0xfff4f2e7}:juce::Colour{0xff605e63});
    body.addColour(.94,rider?juce::Colour{0xffa09f94}:juce::Colour{0xff141316});
    g.setGradientFill(body);g.fillRoundedRectangle(cap,2.5f);
    g.setColour(juce::Colour{0xff44433f});g.drawRoundedRectangle(cap,2.5f,.8f);
    g.setColour(juce::Colours::white.withAlpha(.65f));g.drawHorizontalLine(static_cast<int>(cap.getY()+1),cap.getX()+4,cap.getRight()-4);
    if(rider){
        for(int offset:{-32,-25,-18,-11,11,18,25,32}){
            const float y=cap.getCentreY()+offset;
            g.setColour(juce::Colour{0xff727468});g.drawLine(cap.getX()+4,y,cap.getRight()-4,y,1.3f);
            g.setColour(juce::Colour{0xfff8f6e9});g.drawLine(cap.getX()+4,y+1.5f,cap.getRight()-4,y+1.5f,1.1f);
        }
        juce::ColourGradient edge(juce::Colours::black.withAlpha(.27f),cap.getTopLeft(),juce::Colours::transparentBlack,cap.getCentre(),false);
        g.setGradientFill(edge);g.fillRoundedRectangle(cap,2.5f);
    }
    g.setColour(rider?juce::Colour{0xff9d423b}:juce::Colour{0xffa4a19f});
    g.drawLine(cap.getX()+2,cap.getCentreY(),cap.getRight()-2,cap.getCentreY(),rider?1.4f:1.f);
}
void HardwareLookAndFeel::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& s) {
    const bool vertical=style==juce::Slider::LinearVertical, rider=s.getName()=="rider";
    if(s.getName()=="idle") {juce::Path arrow;arrow.addTriangle(x+3.f,pos-5,x+3.f,pos+5,x+11.f,pos);g.setColour(juce::Colour{0xff252832});g.fillPath(arrow);return;}
    if(vertical) drawFaderTrack(g,x+w*.5f,static_cast<float>(y),static_cast<float>(y+h));
    else {
        auto bar=juce::Rectangle<float>(static_cast<float>(x),y+h*.5f-5,static_cast<float>(w),10);
        g.setColour(juce::Colour{0xff09090b});g.fillRect(bar.expanded(2));
        g.setGradientFill(juce::ColourGradient(juce::Colour{0xff24232a},bar.getTopLeft(),juce::Colour{0xff65616b},bar.getBottomLeft(),false));g.fillRect(bar);
        const float level=static_cast<float>(s.getProperties().getWithDefault("level",-120.f));
        g.setGradientFill(juce::ColourGradient(juce::Colour{0xffe5df72},bar.getTopLeft(),juce::Colour{0xff89802a},bar.getBottomLeft(),false));
        g.fillRect(bar.withWidth(juce::jlimit(0.f,1.f,(level+42)/42)*w));
        auto handle=juce::Rectangle<float>(pos-13,y+h*.5f-11,26,22);
        g.setColour(juce::Colours::black.withAlpha(.6f));g.fillRect(handle.translated(2,2));
        g.setGradientFill(juce::ColourGradient(juce::Colour{0xffd5cace}.withAlpha(.92f),handle.getTopLeft(),juce::Colour{0xff777173}.withAlpha(.95f),handle.getBottomLeft(),false));g.fillRect(handle);
        g.setColour(juce::Colour{0xffece7e7}.withAlpha(.7f));g.drawRect(handle,1);g.setColour(juce::Colour{0xffa16c6f});g.drawVerticalLine(static_cast<int>(pos),handle.getY()+1,handle.getBottom()-1);return;
    }
    drawFaderCap(g,{x+w*.5f-(rider?37.f:27.f),pos-(rider?43.f:15.f),rider?74.f:54.f,rider?86.f:30.f},rider);
}
void HardwareLookAndFeel::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&) {
 auto area=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(12);float d=std::min(area.getWidth(),area.getHeight());auto r=area.withSizeKeepingCentre(d,d);auto c=r.getCentre();
 for(int spread=5;spread>=1;--spread) {g.setColour(juce::Colours::black.withAlpha(.08f));g.fillEllipse(r.expanded((float)spread).translated(2,4));}
 g.setColour(juce::Colour{0xffb7b3b9});for(int i=0;i<=10;++i) {float a=start+(end-start)*(float)i/10;juce::Point<float> u{std::sin(a),-std::cos(a)};g.drawLine({c+u*(d/2+4),c+u*(d/2+8)},i%5==0?1.4f:.8f);}
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xffd4d8ce},r.getTopLeft(),juce::Colour{0xff343139},r.getBottomRight(),false));g.fillEllipse(r);
 auto face=r.reduced(3);g.setColour(juce::Colour{0xff101015});g.fillEllipse(face);
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xff42414c},face.getTopLeft(),juce::Colour{0xff22212a},face.getBottomRight(),false));g.fillEllipse(face.reduced(2));
 g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(face.reduced(4),.7f);
 float a=start+(end-start)*position;juce::Point<float> u{std::sin(a),-std::cos(a)};g.setColour(ivory);g.drawLine({c+u*(d*.12f),c+u*(d/2-6)},2.5f);
}
