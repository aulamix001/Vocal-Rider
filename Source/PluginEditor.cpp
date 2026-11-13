#include "PluginEditor.h"
namespace {
const juce::Colour ivory{0xffeeecdf};
void setParameter(juce::AudioProcessorValueTreeState& state, const char* id, float value) {
    auto* parameter = state.getParameter(id); parameter->beginChangeGesture();
    parameter->setValueNotifyingHost(parameter->convertTo0to1(value)); parameter->endChangeGesture();
}
}
void RangeBand::paint(juce::Graphics& g) {
    const float x=getWidth()*.5f,height=getHeight()-70.f;
    const float low=processor.state.getRawParameterValue("lower")->load(),high=processor.state.getRawParameterValue("upper")->load();
    const auto position=[&](float db){return 35+(12-db)*height/24;};
    g.setGradientFill(juce::ColourGradient(juce::Colour{0xffb7ad78}.withAlpha(.35f),x-20,0,juce::Colour{0xffefdf9f}.withAlpha(.14f),x+20,0,false));
    g.fillRect(x-20,position(high),40.f,std::max(0.f,position(low)-position(high)));
    HardwareLookAndFeel::drawFaderTrack(g,x,35,height+35);
    for(float value:{low,high}) HardwareLookAndFeel::drawFaderCap(g,{x-27,position(value)-15,54,30},false);
}
void RangeBand::mouseDown(const juce::MouseEvent& event) {
    const float value = 12 - (event.position.y-35)*24/(getHeight()-70);
    const float low=processor.state.getRawParameterValue("lower")->load(), high=processor.state.getRawParameterValue("upper")->load();
    lowerHandle = std::abs(value-low) < std::abs(value-high);
    dragging = processor.state.getParameter(lowerHandle ? "lower" : "upper");
    dragging->beginChangeGesture(); mouseDrag(event);
}
void RangeBand::mouseDrag(const juce::MouseEvent& event) {
    if (!dragging) return;
    float value = juce::jlimit(-12.0f,12.0f,12-(event.position.y-35)*24/(getHeight()-70));
    if (lowerHandle) value = std::min(value, processor.state.getRawParameterValue("upper")->load()-1);
    else value = std::max(value, processor.state.getRawParameterValue("lower")->load()+1);
    dragging->setValueNotifyingHost(dragging->convertTo0to1(value)); repaint();
}
void RangeBand::mouseUp(const juce::MouseEvent&) { if (dragging) dragging->endChangeGesture(); dragging=nullptr; }
void VocalBalanceAudioProcessorEditor::buildPanel() {
    panel=juce::Image(juce::Image::RGB,420,784,false);
    juce::Random random(0x6b31a);
    { juce::Image::BitmapData pixels(panel,juce::Image::BitmapData::writeOnly);
      for(int y=0;y<784;++y)for(int x=0;x<420;++x){
        const double grain=random.nextDouble()-.5;
        if(y>=728){
            const double leather=std::sin(x*1.7+std::sin(y*.83)*3)*std::cos(y*1.9+x*.3);
            const int v=juce::jlimit(0,255,17+static_cast<int>(grain*8+leather*5));
            pixels.setPixelColour(x,y,juce::Colour(static_cast<juce::uint8>(v),static_cast<juce::uint8>(v),static_cast<juce::uint8>(v+2)));
        }else if(y>=225 && x>=8 && x<412){
            const double reflection=8*std::cos(x*.019)+11*std::exp(-std::pow((x-225)/88.,2));
            const double patina=2.5*std::sin(x*.043+y*.079)+1.7*std::cos(y*.031-x*.017);
            const double brushing=(y%3==0?-1.8:.4)+grain*5;
            const int v=juce::jlimit(0,250,187+static_cast<int>(reflection+patina+brushing));
            pixels.setPixelColour(x,y,juce::Colour(static_cast<juce::uint8>(v+1),static_cast<juce::uint8>(v),static_cast<juce::uint8>(v+3)));
        }else{
            const double brushed=(y%4==0?-1.5:0)+grain*5+2*std::sin(x*.02+y*.04);
            const int v=33+static_cast<int>(brushed);
            pixels.setPixelColour(x,y,juce::Colour(static_cast<juce::uint8>(v),static_cast<juce::uint8>(v-1),static_cast<juce::uint8>(v+5)));
        }
      }
    }
    juce::Graphics g(panel);
    for(int i=0;i<240;++i){
        int y=40+random.nextInt(178),x=9+random.nextInt(385),length=4+random.nextInt(35);
        g.setColour((i%2?juce::Colours::black:juce::Colours::white).withAlpha(.018f));g.drawHorizontalLine(y,static_cast<float>(x),static_cast<float>(std::min(x+length,410)));
    }
    g.setColour(juce::Colours::white.withAlpha(.33f));g.drawHorizontalLine(225,8,412);g.drawVerticalLine(8,225,728);
    g.setColour(juce::Colours::black.withAlpha(.65f));g.drawHorizontalLine(727,8,412);g.drawVerticalLine(411,225,728);
    g.setColour(juce::Colour{0xff08080a});g.fillRect(8,8,404,28);g.setColour(juce::Colour{0xff77737b});g.drawRect(8,8,404,28);
    g.setGradientFill(juce::ColourGradient(juce::Colours::black.withAlpha(.65f),0,728,juce::Colours::transparentBlack,0,744,false));g.fillRect(0,728,420,16);
    g.setColour(juce::Colours::black);g.drawRect(1,1,418,782,2);
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
    const char* modes[]={"OFF","WRITE","READ"}; const juce::Colour colours[]={juce::Colour{0xffd1cf7e},juce::Colour{0xff8c343d},juce::Colour{0xff4b873c}};
    for(int i=0;i<3;++i) {auto& button=modeButtons[static_cast<size_t>(i)];button.setName("automation");button.setButtonText(modes[i]);button.setClickingTogglesState(true);button.setRadioGroupId(20);button.setColour(juce::TextButton::buttonColourId,colours[i].darker(.5f));button.setColour(juce::TextButton::buttonOnColourId,colours[i]);button.onClick=[this,i]{setMode(i);};addAndMakeVisible(button);}
    const char* labels[]={"<",">","A / B","Load","Save","?"};
    for(size_t i=0;i<toolbar.size();++i){toolbar[i].setButtonText(labels[i]);toolbar[i].setColour(juce::TextButton::buttonColourId,juce::Colour{0xff222729});addAndMakeVisible(toolbar[i]);}
    toolbar[0].onClick=[this]{processor.history.undo();}; toolbar[1].onClick=[this]{processor.history.redo();};
    bankA=processor.state.copyState();bankB=bankA.createCopy();
    toolbar[2].onClick=[this]{if(comparingB){bankB=processor.state.copyState();processor.state.replaceState(bankA.createCopy());}else{bankA=processor.state.copyState();processor.state.replaceState(bankB.createCopy());}comparingB=!comparingB;toolbar[2].setButtonText(comparingB?"B / A":"A / B");};
    toolbar[3].onClick=[this]{loadPreset();};toolbar[4].onClick=[this]{savePreset();};
    toolbar[5].onClick=[]{juce::AlertWindow::showMessageBoxAsync(juce::MessageBoxIconType::InfoIcon,"Vocal Balance","Set Target, activity sensitivity and gain limits. Send the instrumental to the Music sidechain. Drag Rider to override. Off: automatic; Write: publish Rider to DAW automation; Read: follow its automation. Arm Write/Touch in your DAW to record movements.");};
    setSize(420,784);startTimerHz(30);
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
    presets.setSelectedId(processor.getCurrentProgram()+1,juce::dontSendNotification);
    toolbar[0].setEnabled(processor.history.canUndo());toolbar[1].setEnabled(processor.history.canRedo());range.repaint();repaint();
}
void VocalBalanceAudioProcessorEditor::resized(){
    const int x[]={14,38,160,206,248,290};const int widths[]={22,22,42,38,38,22};
    for(size_t i=0;i<toolbar.size();++i)toolbar[i].setBounds(x[i],14,widths[i],18);
    presets.setBounds(64,14,92,18);bypass.setBounds(316,13,62,20);
    controls[0].setBounds(82,49,255,40);controls[1].setBounds(102,107,90,89);controls[2].setBounds(240,107,90,89);attack.setBounds(23,132,70,39);
    range.setBounds(26,259,104,386);controls[3].setBounds(154,238,112,428);controls[4].setBounds(300,260,86,384);controls[5].setBounds(18,284,25,336);
    for(int i=0;i<3;++i)modeButtons[static_cast<size_t>(i)].setBounds(132+i*52,704,52,23);
}
void VocalBalanceAudioProcessorEditor::paint(juce::Graphics& g){
    g.drawImageAt(panel,0,0);
    auto label=[&](const juce::String& text,int x,int y,int width,bool light=false){g.setColour(light?ivory:juce::Colour{0xff303733});g.setFont(12);g.drawText(text,x,y,width,20,juce::Justification::centred);};
    label("Target",16,50,65,true);label("Sensitivity",145,88,180,true);label("Vocal",107,194,80,true);label("Music",245,194,80,true);
    label("Range",28,239,100);label("Rider",170,239,80);label("Output",309,239,72);label("0",140,103,25,true);label("0",278,103,25,true);
    auto led=[&](float x,float y,float d,bool on){if(on)for(int r=5;r>0;--r){g.setColour(juce::Colour{0xff7bfa3a}.withAlpha(.035f));g.fillEllipse(x-r,y-r,d+2*r,d+2*r);}g.setColour(juce::Colour{0xff07100a});g.fillEllipse(x-1,y-1,d+2,d+2);g.setGradientFill(juce::ColourGradient(on?juce::Colour{0xffaaff63}:juce::Colour{0xff5a6258},x+d*.3f,y+d*.2f,on?juce::Colour{0xff276c12}:juce::Colour{0xff17211a},x+d,y+d,true));g.fillEllipse(x,y,d,d);
        if(d>15){g.saveState();juce::Path lens;lens.addEllipse(x+2,y+2,d-4,d-4);g.reduceClipRegion(lens);g.setColour(juce::Colours::black.withAlpha(.3f));for(int i=3;i<static_cast<int>(d)-1;i+=3){g.drawVerticalLine(static_cast<int>(x)+i,y+1,y+d-1);g.drawHorizontalLine(static_cast<int>(y)+i,x+1,x+d-1);}g.restoreState();g.setColour(juce::Colours::white.withAlpha(.22f));g.drawEllipse(x+.5f,y+.5f,d-1,d-1,1.f);}
    };
    led(25,93,10,processor.vocalActive.load());led(25,111,10,!processor.vocalActive.load());label("Ride",38,88,45,true);label("Idle",38,106,45,true);led(365,101,20,processor.musicActive.load());
    g.setColour(processor.outputLevel.load()>0?juce::Colours::red:juce::Colour{0xff344039});g.fillEllipse(376,244,10,10);
    // Half-dB vernier marks around the rider, coarser marks on the output.
    for(int half=-24;half<=24;++half){
        const float db=half*.5f;const int y=static_cast<int>(294+(12-db)*316/24);
        const bool major=half%12==0;const float length=major?19.f:(half%2==0?12.f:7.f);
        g.setColour(juce::Colour{0xff56525b}.withAlpha(major?.9f:.65f));
        g.drawHorizontalLine(y,164-length,164);g.drawHorizontalLine(y,256,256+length);
        if(half%2==0)g.drawHorizontalLine(y,365,major?380.f:373.f);
        if(major){juce::String value=db>0?"+"+juce::String(db,0):juce::String(db,0);label(value,119,y-10,26);label(value,277,y-10,27);label(value,379,y-10,29);}
    }
    g.setColour(juce::Colour{0xffa16366}.withAlpha(.6f));g.drawHorizontalLine(452,18,398);
    auto readout=[&](int x,float value){g.setColour(juce::Colour{0xff09090a});g.fillRect(x,635,52,18);g.setColour(ivory);g.setFont(11);g.drawText(juce::String(value,1),x,635,52,18,juce::Justification::centred);};
    readout(52,processor.state.getRawParameterValue("upper")->load()-processor.state.getRawParameterValue("lower")->load());readout(184,processor.gainDb.load());readout(318,processor.state.getRawParameterValue("output")->load());
    label("Automation",145,679,130);
    auto screw=[&](float x,float y){g.setGradientFill(juce::ColourGradient(juce::Colour{0xff1b1820},x,y,juce::Colour{0xff807984},x+12,y+12,false));g.fillEllipse(x,y,12,12);g.setColour(juce::Colour{0xff322d36});g.fillEllipse(x+3,y+3,6,6);};
    screw(204,259);screw(204,665);
    g.setColour(ivory);g.setFont(12);g.drawText("VB",379,12,25,22,juce::Justification::centred);
    auto font=g.getCurrentFont();font.setTypefaceName(juce::Font::getDefaultSerifFontName());font.setHeight(25);font.setBold(true);g.setFont(font);g.drawText("Vocal Balance",22,744,375,27,juce::Justification::centredRight);
}
