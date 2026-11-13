#include "PluginEditor.h"
namespace {
const juce::Colour ivory{0xffeeecdf};
void setParameter(juce::AudioProcessorValueTreeState& state, const char* id, float value) {
    auto* parameter = state.getParameter(id); parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value)); parameter->endChangeGesture();
}
}
void RangeBand::paint(juce::Graphics& g) {
    const float x = getWidth() * .5f, height = getHeight() - 24.0f;
    const float low = processor.state.getRawParameterValue("lower")->load(), high = processor.state.getRawParameterValue("upper")->load();
    const auto position = [&](float db) { return 10 + (12 - db) * height / 24; };
    g.setColour(juce::Colour{0xffc4b887}.withAlpha(.4f)); g.fillRect(x-15, position(high), 30.0f, std::max(0.0f, position(low)-position(high)));
    g.setColour(juce::Colour{0xff737773}); g.fillRect(x-6, 10.0f, 12.0f, height);
    g.setColour(juce::Colours::black); g.fillRect(x-2, 10.0f, 4.0f, height);
    for (float value : {low, high}) {
        const float y = position(value); juce::Rectangle<float> cap{x-20,y-9,40,18};
        g.setColour(juce::Colours::black.withAlpha(.3f)); g.fillRect(cap.translated(2,4));
        g.setGradientFill(juce::ColourGradient(juce::Colour{0xff747b76},cap.getTopLeft(),juce::Colour{0xff111917},cap.getBottomLeft(),false)); g.fillRect(cap);
        g.setColour(ivory.withAlpha(.6f)); g.drawHorizontalLine(static_cast<int>(y),x-18,x+18);
    }
}
void RangeBand::mouseDown(const juce::MouseEvent& event) {
    const float value = 12 - (event.position.y-10)*24/(getHeight()-24);
    const float low=processor.state.getRawParameterValue("lower")->load(), high=processor.state.getRawParameterValue("upper")->load();
    lowerHandle = std::abs(value-low) < std::abs(value-high);
    dragging = processor.state.getParameter(lowerHandle ? "lower" : "upper");
    dragging->beginChangeGesture(); mouseDrag(event);
}
void RangeBand::mouseDrag(const juce::MouseEvent& event) {
    if (!dragging) return;
    float value = juce::jlimit(-12.0f,12.0f,12-(event.position.y-10)*24/(getHeight()-24));
    if (lowerHandle) value = std::min(value, processor.state.getRawParameterValue("upper")->load()-1);
    else value = std::max(value, processor.state.getRawParameterValue("lower")->load()+1);
    dragging->setValueNotifyingHost(dragging->convertTo0to1(value)); repaint();
}
void RangeBand::mouseUp(const juce::MouseEvent&) { if (dragging) dragging->endChangeGesture(); dragging=nullptr; }
void VocalBalanceAudioProcessorEditor::buildPanel() {
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

VocalBalanceAudioProcessorEditor::VocalBalanceAudioProcessorEditor(VocalBalanceAudioProcessor& owner)
    : AudioProcessorEditor(owner), processor(owner), range(owner) {
    buildPanel(); setLookAndFeel(&look);
    constexpr const char* ids[] = {"target","vocal","music","rider","output","idle"};
    constexpr const char* help[] = {"Vocal target: -42 to 0 dBFS","Vocal sensitivity: -12 to +12 dB","Music sidechain sensitivity: -12 to +12 dB","Automatic gain; drag for manual control","Output trim: -12 to +12 dB","Rest gain when voice is inactive"};
    for (size_t i=0;i<controls.size();++i) {
        auto& slider=controls[i]; slider.setName(ids[i]); slider.setTextBoxStyle(juce::Slider::NoTextBox,false,0,0);
        slider.setSliderStyle(i==0?juce::Slider::LinearHorizontal:(i==1||i==2?juce::Slider::RotaryHorizontalVerticalDrag:juce::Slider::LinearVertical));
        slider.setTooltip(help[i]); slider.setDoubleClickReturnValue(true,i==0?-21:0); addAndMakeVisible(slider);
        attachments[i]=std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment>(processor.state,ids[i],slider);
    }
    controls[3].onDragStart=[this]{processor.manualRider.store(true);}; controls[3].onDragEnd=[this]{processor.manualRider.store(false);};
    addAndMakeVisible(range);
    for(int i=0;i<3;++i) presets.addItem(processor.getProgramName(i),i+1);
    presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
    presets.onChange=[this]{processor.setCurrentProgram(presets.getSelectedId()-1);}; addAndMakeVisible(presets);
    bypass.setColour(juce::ToggleButton::textColourId,ivory);addAndMakeVisible(bypass);
    bypassAttachment=std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(processor.state,"bypass",bypass);
    attack.setName("attack-switch"); attack.onClick=[this]{setParameter(processor.state,"attack",processor.state.getRawParameterValue("attack")->load()<.5f?1:0);}; addAndMakeVisible(attack);
    const char* modes[]={"OFF","WRITE","READ"}; const juce::Colour colours[]={juce::Colour{0xffc5bd6a},juce::Colour{0xffa84e43},juce::Colour{0xff69a157}};
    for(int i=0;i<3;++i) {auto& button=modeButtons[static_cast<size_t>(i)];button.setButtonText(modes[i]);button.setClickingTogglesState(true);button.setRadioGroupId(20);button.setColour(juce::TextButton::buttonColourId,colours[i].darker(.5f));button.setColour(juce::TextButton::buttonOnColourId,colours[i]);button.onClick=[this,i]{setMode(i);};addAndMakeVisible(button);}
    const char* labels[]={"<",">","A / B","Load","Save","?"};
    for(size_t i=0;i<toolbar.size();++i){toolbar[i].setButtonText(labels[i]);toolbar[i].setColour(juce::TextButton::buttonColourId,juce::Colour{0xff222729});addAndMakeVisible(toolbar[i]);}
    toolbar[0].onClick=[this]{processor.history.undo();}; toolbar[1].onClick=[this]{processor.history.redo();};
    bankA=processor.state.copyState();bankB=bankA.createCopy();
    toolbar[2].onClick=[this]{if(comparingB){bankB=processor.state.copyState();processor.state.replaceState(bankA.createCopy());}else{bankA=processor.state.copyState();processor.state.replaceState(bankB.createCopy());}comparingB=!comparingB;toolbar[2].setButtonText(comparingB?"B / A":"A / B");};
    toolbar[3].onClick=[this]{loadPreset();};toolbar[4].onClick=[this]{savePreset();};
    toolbar[5].onClick=[]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Vocal Balance","Set Target, activity sensitivity and gain limits. Send the instrumental to the Music sidechain. Drag Rider to override. Off: automatic; Write: publish Rider to DAW automation; Read: follow its automation. Arm Write/Touch in your DAW to record movements.");};
    setSize(420,740);startTimerHz(30);
}
VocalBalanceAudioProcessorEditor::~VocalBalanceAudioProcessorEditor(){stopTimer();setLookAndFeel(nullptr);}
void VocalBalanceAudioProcessorEditor::setMode(int mode){setParameter(processor.state,"mode",static_cast<float>(mode));}
void VocalBalanceAudioProcessorEditor::loadPreset(){
    chooser=std::make_unique<juce::FileChooser>("Load Vocal Balance preset",juce::File{},"*.vbpreset");
    const juce::Component::SafePointer<VocalBalanceAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::openMode|juce::FileBrowserComponent::canSelectFiles,[safe](const juce::FileChooser& selected){if(!safe)return;if(auto xml=juce::XmlDocument::parse(selected.getResult()))if(xml->hasTagName("VocalBalanceState2"))safe->processor.state.replaceState(juce::ValueTree::fromXml(*xml));});
}
void VocalBalanceAudioProcessorEditor::savePreset(){
    chooser=std::make_unique<juce::FileChooser>("Save Vocal Balance preset",juce::File::getSpecialLocation(juce::File::userDocumentsDirectory).getChildFile("VocalBalance.vbpreset"),"*.vbpreset");
    const juce::Component::SafePointer<VocalBalanceAudioProcessorEditor> safe(this);
    chooser->launchAsync(juce::FileBrowserComponent::saveMode|juce::FileBrowserComponent::canSelectFiles|juce::FileBrowserComponent::warnAboutOverwriting,[safe](const juce::FileChooser& selected){if(!safe||selected.getResult()==juce::File{})return;if(auto xml=safe->processor.state.copyState().createXml())if(!xml->writeTo(selected.getResult().withFileExtension("vbpreset")))juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon,"Preset","Could not save the preset.");});
}
void VocalBalanceAudioProcessorEditor::timerCallback(){
    if(!processor.manualRider.load())controls[3].setValue(processor.gainDb.load(),juce::dontSendNotification);
    controls[0].getProperties().set("level",processor.inputLevel.load());
    attack.setButtonText(processor.state.getRawParameterValue("attack")->load()<.5f?"Fast":"Slow");
    const int mode=static_cast<int>(processor.state.getRawParameterValue("mode")->load());
    for(int i=0;i<3;++i)modeButtons[static_cast<size_t>(i)].setToggleState(i==mode,juce::dontSendNotification);
    toolbar[0].setEnabled(processor.history.canUndo());toolbar[1].setEnabled(processor.history.canRedo());range.repaint();repaint();
}
void VocalBalanceAudioProcessorEditor::resized(){
    const int x[]={14,38,160,206,248,290};const int widths[]={22,22,42,38,38,22};
    for(size_t i=0;i<toolbar.size();++i)toolbar[i].setBounds(x[i],14,widths[i],18);
    presets.setBounds(64,14,92,18);bypass.setBounds(316,13,62,20);
    controls[0].setBounds(82,49,255,40);controls[1].setBounds(110,107,90,89);controls[2].setBounds(248,107,90,89);attack.setBounds(23,132,70,39);
    range.setBounds(36,269,84,284);controls[3].setBounds(168,269,84,280);controls[4].setBounds(312,269,62,280);controls[5].setBounds(12,269,25,280);
    for(int i=0;i<3;++i)modeButtons[static_cast<size_t>(i)].setBounds(132+i*52,663,52,23);
}
void VocalBalanceAudioProcessorEditor::paint(juce::Graphics& g){
    g.drawImageAt(panel,0,0);
    auto label=[&](const juce::String& text,int x,int y,int width,bool light=false){g.setColour(light?ivory:juce::Colour{0xff303733});g.setFont(12);g.drawText(text,x,y,width,20,juce::Justification::centred);};
    label("Target",16,50,65,true);label("Sensitivity",145,88,180,true);label("Vocal",115,194,80,true);label("Music",253,194,80,true);
    label("Range",28,239,100);label("Rider",170,239,80);label("Output",309,239,72);label("0",140,103,25,true);label("0",278,103,25,true);
    auto led=[&](float x,float y,float d,bool on){if(on)for(int r=5;r>0;--r){g.setColour(juce::Colour{0xff7bfa3a}.withAlpha(.035f));g.fillEllipse(x-r,y-r,d+2*r,d+2*r);}g.setColour(juce::Colour{0xff07100a});g.fillEllipse(x-1,y-1,d+2,d+2);g.setGradientFill(juce::ColourGradient(on?juce::Colour{0xffaaff63}:juce::Colour{0xff5a6258},x+d*.3f,y+d*.2f,on?juce::Colour{0xff276c12}:juce::Colour{0xff17211a},x+d,y+d,true));g.fillEllipse(x,y,d,d);};
    led(25,93,10,processor.vocalActive.load());led(25,111,10,!processor.vocalActive.load());label("Ride",38,88,45,true);label("Idle",38,106,45,true);led(376,144,15,processor.musicActive.load());
    g.setColour(processor.outputLevel.load()>0?juce::Colours::red:juce::Colour{0xff344039});g.fillEllipse(376,244,10,10);
    for(int db=-12;db<=12;++db){const int y=280+(12-db)*260/24;g.setColour(juce::Colour{0xff4d544f});g.drawHorizontalLine(y,149,165);g.drawHorizontalLine(y,255,271);if(db%6==0){juce::String value=db>0?"+"+juce::String(db):juce::String(db);label(value,19,y-10,25);label(value,123,y-10,25);label(value,273,y-10,28);}}
    g.setColour(juce::Colour{0xff9c6758}.withAlpha(.5f));g.drawHorizontalLine(410,20,392);
    auto readout=[&](int x,float value){g.setColour(juce::Colour{0xff101510});g.fillRect(x,569,52,18);g.setColour(ivory);g.drawText(juce::String(value,1),x,569,52,18,juce::Justification::centred);};
    readout(52,processor.state.getRawParameterValue("upper")->load()-processor.state.getRawParameterValue("lower")->load());readout(184,processor.gainDb.load());readout(318,processor.state.getRawParameterValue("output")->load());label("Effective range",30,593,96);label("Automation",145,638,130);
    g.setColour(ivory);g.setFont(12);g.drawText("VB",379,12,25,22,juce::Justification::centred);
    auto font=g.getCurrentFont();font.setTypefaceName(juce::Font::getDefaultSerifFontName());font.setHeight(25);font.setBold(true);g.setFont(font);g.drawText("Vocal Balance",22,707,375,27,juce::Justification::centredRight);
}
