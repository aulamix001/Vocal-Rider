#pragma once
#include "Plugin.h"
class HardwareTheme final : public juce::LookAndFeel_V4 {
public:
 void drawButtonBackground(juce::Graphics&,juce::Button&,const juce::Colour&,bool,bool) override;
 void drawButtonText(juce::Graphics&,juce::TextButton&,bool,bool) override;
 void drawLinearSlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider::SliderStyle,juce::Slider&) override;
 void drawRotarySlider(juce::Graphics&,int,int,int,int,float,float,float,juce::Slider&) override;
};
class RangeLane final : public juce::Component {
public:
 RangeLane(juce::Slider& low,juce::Slider& high):minimum(low),maximum(high) {}
 void paint(juce::Graphics&) override;
 void mouseDown(const juce::MouseEvent&) override;
 void mouseDrag(const juce::MouseEvent&) override;
 void mouseUp(const juce::MouseEvent&) override;
private:
 juce::Slider& minimum; juce::Slider& maximum; juce::Slider* dragged=nullptr;
};
class BalanceEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
 explicit BalanceEditor(VocalBalance&);
 ~BalanceEditor() override { stopTimer(); setLookAndFeel(nullptr); }
 void paint(juce::Graphics&) override;
 void resized() override;
private:
 void timerCallback() override;
 VocalBalance& processor;
 HardwareTheme theme;
 std::array<juce::Slider,8> sliders;
 RangeLane range{sliders[1],sliders[2]};
 std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>,8> attachments;
 juce::Image panel;
 void buildPanel();
 std::array<juce::TextButton,6> toolbar;
 juce::TextButton speed{"Slow"};
 juce::ValueTree comparisonA,comparisonB;
 bool bankB=false;
 std::unique_ptr<juce::FileChooser> chooser;
 juce::ToggleButton bypass{"Bypass"};
 std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
 std::array<juce::TextButton,3> automationButtons;
 juce::ComboBox presets,attack,mode;
 std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attackAttachment,modeAttachment;
};
