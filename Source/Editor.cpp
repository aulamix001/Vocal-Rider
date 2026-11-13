#include "Editor.h"
namespace { const juce::Colour dark{0xff20282c}, ivory{0xffeeecdf}; }
void HardwareTheme::drawButtonBackground(juce::Graphics& g,juce::Button& b,const juce::Colour& colour,bool hover,bool down) {
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
void HardwareTheme::drawButtonText(juce::Graphics& g,juce::TextButton& b,bool hover,bool down) {
 if(b.getName()=="attack-switch") {g.setFont(11);g.setColour(ivory.withAlpha(b.getButtonText()=="Fast"?1.f:.5f));g.drawText("Fast",20,1,40,17,juce::Justification::centredLeft);g.setColour(ivory.withAlpha(b.getButtonText()=="Slow"?1.f:.5f));g.drawText("Slow",20,19,40,17,juce::Justification::centredLeft);return;}
 juce::LookAndFeel_V4::drawButtonText(g,b,hover,down);
}
void HardwareTheme::drawLinearSlider(juce::Graphics& g,int x,int y,int w,int h,float pos,float,float,juce::Slider::SliderStyle style,juce::Slider& s) {
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
void HardwareTheme::drawRotarySlider(juce::Graphics& g,int x,int y,int w,int h,float position,float start,float end,juce::Slider&) {
 auto area=juce::Rectangle<float>((float)x,(float)y,(float)w,(float)h).reduced(12);float d=std::min(area.getWidth(),area.getHeight());auto r=area.withSizeKeepingCentre(d,d);auto c=r.getCentre();
 for(int spread=5;spread>=1;--spread) {g.setColour(juce::Colours::black.withAlpha(.08f));g.fillEllipse(r.expanded((float)spread).translated(2,4));}
 g.setColour(juce::Colour{0xffadb4b2});for(int i=0;i<=10;++i) {float a=start+(end-start)*(float)i/10;juce::Point<float> u{std::sin(a),-std::cos(a)};g.drawLine({c+u*(d/2+4),c+u*(d/2+8)},i%5==0?1.4f:.8f);}
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xffd4d8ce},r.getTopLeft(),juce::Colour{0xff343c3d},r.getBottomRight(),false));g.fillEllipse(r);
 auto face=r.reduced(3);g.setColour(juce::Colour{0xff101618});g.fillEllipse(face);
 g.setGradientFill(juce::ColourGradient(juce::Colour{0xff596267},face.getTopLeft(),juce::Colour{0xff1e282d},face.getBottomRight(),false));g.fillEllipse(face.reduced(2));
 g.setColour(juce::Colours::white.withAlpha(.12f));g.drawEllipse(face.reduced(4),.7f);
 float a=start+(end-start)*position;juce::Point<float> u{std::sin(a),-std::cos(a)};g.setColour(ivory);g.drawLine({c+u*(d*.12f),c+u*(d/2-6)},2.5f);
}

void RangeLane::paint(juce::Graphics& g) {
 const float h=(float)getHeight()-28, x=(float)getWidth()/2;
 float top=8+(12-(float)maximum.getValue())*h/24,bottom=8+(12-(float)minimum.getValue())*h/24;
 g.setColour(juce::Colour{0xffc4b887}.withAlpha(.4f));g.fillRect(x-15,top,30.f,std::max(0.f,bottom-top));
 g.setColour(juce::Colour{0xff777b73});g.fillRect(x-7,8.f,14.f,h); g.setColour(juce::Colours::black);g.fillRect(x-2,8.f,4.f,h);
 for(double value:{minimum.getValue(),maximum.getValue()}) {
  float y=8+(12-(float)value)*h/24; juce::Rectangle<float> cap{x-20,y-9,40,18};
  g.setColour(juce::Colours::black.withAlpha(0.35f));g.fillRect(cap.translated(2,3));
  g.setGradientFill(juce::ColourGradient(juce::Colour{0xff6f7470},cap.getTopLeft(),juce::Colour{0xff101613},cap.getBottomLeft(),false));g.fillRect(cap);
  g.setColour(juce::Colour{0xffa8b0a6});g.drawHorizontalLine((int)y,x-18,x+18);
 }
}
void RangeLane::mouseDown(const juce::MouseEvent& e) {
 auto value=12.0-24.0*(e.position.y-8)/(getHeight()-28);
 dragged=std::abs(value-minimum.getValue())<std::abs(value-maximum.getValue())?&minimum:&maximum;
 if(dragged->onDragStart) dragged->onDragStart();
 mouseDrag(e);
}
void RangeLane::mouseDrag(const juce::MouseEvent& e) {
 if(!dragged) return;
 double v=juce::jlimit(-12.0,12.0,12.0-24.0*(e.position.y-8)/(getHeight()-28));
 if(dragged==&minimum) v=std::min(v,maximum.getValue()-1); else v=std::max(v,minimum.getValue()+1);
 dragged->setValue(v,juce::sendNotificationSync); repaint();
}
void RangeLane::mouseUp(const juce::MouseEvent&) { if(dragged && dragged->onDragEnd) dragged->onDragEnd(); dragged=nullptr; }

void BalanceEditor::buildPanel() {
 panel=juce::Image(juce::Image::RGB,420,740,false);
 juce::Random random(0x6b31a);
 juce::Image::BitmapData pixels(panel,juce::Image::BitmapData::writeOnly);
 for(int y=0;y<740;++y) for(int x=0;x<420;++x) {
  int noise=random.nextInt(9)-4;
  if(y>=702) {
   double pebble=std::sin(x*.8+std::sin(y*.6)*2)*std::cos(y*.9+x*.1);
   auto v=(juce::uint8)juce::jlimit(0,255,15+noise+(int)(pebble*6));pixels.setPixelColour(x,y,juce::Colour(v,v,v));
  } else if(y>=225 && x>=10 && x<410) {
   double reflection=18*std::cos(x*.011)+15*std::exp(-std::pow((x-240)/65.,2));
   int grain=(y%3==0?-4:1)+noise/2;
   auto v=(juce::uint8)juce::jlimit(0,255,195+(int)reflection-y/80+grain);pixels.setPixelColour(x,y,juce::Colour(v,v,(juce::uint8)std::max(0,(int)v-2)));
  } else {
   int brushed=(y%4==0?-2:0)+noise/2;
   pixels.setPixelColour(x,y,juce::Colour((juce::uint8)(29+brushed),(juce::uint8)(36+brushed),(juce::uint8)(40+brushed)));
  }
 }
 juce::Graphics g(panel);
 for(int i=0;i<140;++i) {
  int y=40+random.nextInt(178),x=10+random.nextInt(375),length=5+random.nextInt(35);
  g.setColour((i%2?juce::Colours::black:juce::Colours::white).withAlpha(.025f));g.drawHorizontalLine(y,(float)x,(float)std::min(x+length,410));
 }
 g.setColour(juce::Colours::white.withAlpha(.35f));g.drawHorizontalLine(225,10,410);g.drawVerticalLine(10,225,702);
 g.setColour(juce::Colours::black.withAlpha(.6f));g.drawHorizontalLine(701,10,410);g.drawVerticalLine(409,225,702);
 g.setColour(juce::Colour{0xff080b0c});g.fillRect(10,9,400,28);
 g.setColour(juce::Colour{0xff6c7474});g.drawRect(10,9,400,28);
}

BalanceEditor::BalanceEditor(VocalBalance& p):AudioProcessorEditor(p),processor(p) {
 buildPanel();
 setLookAndFeel(&theme);
 theme.setColour(juce::Slider::textBoxTextColourId,juce::Colours::white); theme.setColour(juce::Slider::textBoxBackgroundColourId,juce::Colour{0xff101513}); theme.setColour(juce::Slider::textBoxOutlineColourId,juce::Colours::transparentBlack);
 const char* ids[]={"target","minimum","maximum","gate","output","music","idle","rider"};
 for(size_t i=0;i<8;++i) {
  auto& s=sliders[i]; s.setName(ids[i]); s.setSliderStyle(i==0?juce::Slider::LinearHorizontal:i==3||i==5?juce::Slider::RotaryHorizontalVerticalDrag:juce::Slider::LinearVertical); s.setTextBoxStyle(juce::Slider::TextBoxBelow,false,58,18); s.setDoubleClickReturnValue(true,i==0?-21.0:i==1?-6.0:i==2?6.0:0.0); s.setTooltip(ids[i]); addAndMakeVisible(s);
  attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(p.state,ids[i],s);
 }
 sliders[0].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); sliders[3].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); sliders[5].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 sliders[4].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); sliders[6].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0); sliders[7].setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
 sliders[1].onDragStart=[this]{processor.state.getParameter("minimum")->beginChangeGesture();};
 sliders[1].onDragEnd=[this]{processor.state.getParameter("minimum")->endChangeGesture();};
 sliders[2].onDragStart=[this]{processor.state.getParameter("maximum")->beginChangeGesture();};
 sliders[2].onDragEnd=[this]{processor.state.getParameter("maximum")->endChangeGesture();};
 sliders[1].setVisible(false); sliders[2].setVisible(false); addAndMakeVisible(range);
 sliders[7].onDragStart=[this]{processor.manualTouch.store(true);}; sliders[7].onDragEnd=[this]{processor.manualTouch.store(false);};
 bypass.setColour(juce::ToggleButton::textColourId,ivory); addAndMakeVisible(bypass); bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(p.state,"bypass",bypass);
 presets.addItem("Default",1); presets.addItem("Gentle",2); presets.addItem("Firm",3); presets.setSelectedId(p.getCurrentProgram()+1,juce::dontSendNotification); presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);}; addAndMakeVisible(presets);
 attack.addItem("Fast",1); attack.addItem("Slow",2); addAndMakeVisible(attack); attackAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"response",attack);
 mode.addItem("OFF",1); mode.addItem("WRITE",2); mode.addItem("READ",3); addAndMakeVisible(mode); modeAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment>(p.state,"mode",mode);
 const char* modes[]={"OFF","WRITE","READ"}; const juce::Colour colours[]={juce::Colour{0xffc5bd6a},juce::Colour{0xffa84e43},juce::Colour{0xff69a157}};
 for(int i=0;i<3;++i) { auto& b=automationButtons[(size_t)i]; b.setButtonText(modes[i]); b.setClickingTogglesState(true); b.setRadioGroupId(42); b.setColour(juce::TextButton::buttonColourId,colours[i].darker(0.5f)); b.setColour(juce::TextButton::buttonOnColourId,colours[i]); b.onClick=[this,i]{mode.setSelectedId(i+1,juce::sendNotificationSync);}; addAndMakeVisible(b); }
 mode.setVisible(false); attack.setVisible(false);
 comparisonA=p.state.copyState(); comparisonB=comparisonA.createCopy();
 const char* tools[]={"<",">","A / B","Load","Save","?"};
 for(size_t i=0;i<toolbar.size();++i) {auto& b=toolbar[i];b.setButtonText(tools[i]);b.setColour(juce::TextButton::buttonColourId,juce::Colour{0xff222729});b.setColour(juce::TextButton::textColourOffId,ivory);addAndMakeVisible(b);}
 toolbar[0].onClick=[this]{processor.history.undo();}; toolbar[1].onClick=[this]{processor.history.redo();};
 toolbar[2].onClick=[this]{
  if(bankB) {comparisonB=processor.state.copyState();processor.state.replaceState(comparisonA.createCopy());}
  else {comparisonA=processor.state.copyState();processor.state.replaceState(comparisonB.createCopy());}
  bankB=!bankB;toolbar[2].setButtonText(bankB?"B / A":"A / B");
 };
 toolbar[3].onClick=[this]{
  chooser=std::make_unique<juce::FileChooser>("Load Vocal Balance preset",juce::File{},"*.vbpreset");
  const juce::Component::SafePointer<BalanceEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& fc){
   if(!safe) return;
   auto file=fc.getResult();if(!file.existsAsFile()) return;
   if(auto xml=juce::XmlDocument::parse(file)) {if(xml->hasTagName("VocalBalance")) safe->processor.state.replaceState(juce::ValueTree::fromXml(*xml));}
  });
 };
 toolbar[4].onClick=[this]{
  chooser=std::make_unique<juce::FileChooser>("Save Vocal Balance preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("VocalBalance.vbpreset"),"*.vbpreset");
  const juce::Component::SafePointer<BalanceEditor> safe(this);
  chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& fc){
   if(!safe) return;
   auto file=fc.getResult();if(file==juce::File{}) return;
   if(auto xml=safe->processor.state.copyState().createXml()) {if(!xml->writeTo(file.withFileExtension("vbpreset"))) juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Save preset","Could not write preset file.");}
  });
 };
 toolbar[5].onClick=[]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Vocal Balance","Target sets vocal RMS level. Vocal adjusts activity sensitivity. Send music to the sidechain for Music sensitivity. Range limits gain; Idle sets the rest position. Drag Rider for manual control. Off uses automatic riding; Write publishes Rider to the host; Read follows host automation. Arm Write/Touch in your DAW to record. Independent implementation; no affiliation with Waves.");};
 speed.setName("attack-switch");
 speed.onClick=[this]{attack.setSelectedId(attack.getSelectedId()==1?2:1,juce::sendNotificationSync);};addAndMakeVisible(speed);
 bypass.setButtonText("Byp");
 setSize(420,740); startTimerHz(30);
}
void BalanceEditor::timerCallback() { if(!processor.manualTouch.load()) sliders[7].setValue(processor.correction.load(),juce::dontSendNotification); sliders[0].getProperties().set("level",processor.inputLevel.load());
 for(int i=0;i<3;++i) automationButtons[(size_t)i].setToggleState(mode.getSelectedId()==i+1,juce::dontSendNotification);
 speed.setButtonText(attack.getSelectedId()==1?"Fast":"Slow");toolbar[0].setEnabled(processor.history.canUndo());toolbar[1].setEnabled(processor.history.canRedo());range.repaint(); repaint(); }
void BalanceEditor::resized() {
 range.setBounds(36,269,84,294);
 toolbar[0].setBounds(14,14,22,18);toolbar[1].setBounds(38,14,22,18);presets.setBounds(64,14,92,18);
 toolbar[2].setBounds(160,14,42,18);toolbar[3].setBounds(206,14,38,18);toolbar[4].setBounds(248,14,38,18);toolbar[5].setBounds(290,14,22,18);bypass.setBounds(316,13,62,20);
 speed.setBounds(23,132,70,39);
 sliders[0].setBounds(82,49,255,40); sliders[3].setBounds(110,107,90,89); sliders[5].setBounds(248,107,90,89); attack.setBounds(23,136,72,26);
 sliders[1].setBounds(42,269,54,147); sliders[2].setBounds(42,416,54,147);
 // Independent limit handles share the range column; top controls maximum.
 sliders[2].setBounds(76,269,54,294); sliders[1].setBounds(22,269,54,294);
 sliders[7].setBounds(168,269,84,280); sliders[4].setBounds(312,269,62,280);
 sliders[6].setBounds(12,269,25,280); mode.setBounds(145,662,130,26); for(int i=0;i<3;++i) automationButtons[(size_t)i].setBounds(132+i*52,663,52,23);
}
void BalanceEditor::paint(juce::Graphics& g) {
 g.drawImageAt(panel,0,0);
 auto label=[&](const juce::String& t,int x,int y,int w,bool light=false){g.setColour(light?ivory:juce::Colour{0xff303733});g.setFont(12);g.drawText(t,x,y,w,20,juce::Justification::centred);};
 label("Target",16,50,65,true); label("Sensitivity",145,88,180,true); label("Vocal",115,194,80,true); label("Music",253,194,80,true);
 label("Range",28,239,100); label("Rider",170,239,80); label("Output",309,239,72);
 bool active=processor.inputLevel.load()>-55-processor.state.getRawParameterValue("gate")->load();
 auto led=[&](float x,float y,float d,bool on) {
  if(on) for(int spread=6;spread>0;--spread) {g.setColour(juce::Colour{0xff7bfa3a}.withAlpha(.025f));g.fillEllipse(x-spread,y-spread,d+2*spread,d+2*spread);}
  g.setColour(juce::Colour{0xff07100a});g.fillEllipse(x-1,y-1,d+2,d+2);
  g.setGradientFill(juce::ColourGradient(on?juce::Colour{0xffaaff63}:juce::Colour{0xff5a6258},x+d*.35f,y+d*.25f,on?juce::Colour{0xff276c12}:juce::Colour{0xff17211a},x+d,y+d,true));g.fillEllipse(x,y,d,d);
  juce::Path clip;clip.addEllipse(x,y,d,d);g.saveState();g.reduceClipRegion(clip);g.setColour(juce::Colours::black.withAlpha(.2f));for(float k=2;k<d;k+=3) {g.drawLine(x+k,y,x+k,y+d,.5f);g.drawLine(x,y+k,x+d,y+k,.5f);}g.restoreState();
 };
 led(25,93,10,active);led(25,111,10,false);label("Ride",38,88,45,true);label("Idle",38,106,45,true);
 led(376,144,15,processor.musicActive.load());
 g.setColour(processor.outputLevel.load()>0?juce::Colours::red:juce::Colour{0xff344039});g.fillEllipse(376,244,10,10);
 label("0",140,103,25,true);label("0",278,103,25,true);
 g.setColour(ivory);g.setFont(12);g.drawText("VB",379,12,25,22,juce::Justification::centred);
 for(int v=-12;v<=12;++v) { float y=280+(12-v)*(260.f/24); g.setColour(juce::Colour{0xff4d544f}); g.drawHorizontalLine((int)y,149,165);g.drawHorizontalLine((int)y,255,271); if(v%6==0) { label(v>0?"+"+juce::String(v):juce::String(v),19,(int)y-10,25); label(v>0?"+"+juce::String(v):juce::String(v),123,(int)y-10,25); label(v>0?"+"+juce::String(v):juce::String(v),273,(int)y-10,28); } }
 float zero=410;g.setColour(juce::Colour{0xff9c6758}.withAlpha(0.5f));g.drawHorizontalLine((int)zero,20,392);
 auto readout=[&](int x,double value) {g.setColour(juce::Colour{0xff101510});g.fillRect(x,569,52,18);g.setColour(ivory);g.setFont(12);g.drawText(juce::String(value,1),x,569,52,18,juce::Justification::centred);};
 readout(52,std::abs(sliders[2].getValue()-sliders[1].getValue())); readout(184,processor.correction.load()); readout(318,sliders[4].getValue());
 label("Effective range",30,593,96); label("Automation",145,638,130);
 g.setColour(ivory);auto brand=g.getCurrentFont();brand.setTypefaceName(juce::Font::getDefaultSerifFontName());brand.setHeight(25.f);brand.setBold(true);g.setFont(brand);g.drawText("Vocal Balance",22,707,375,27,juce::Justification::centredRight);
}
