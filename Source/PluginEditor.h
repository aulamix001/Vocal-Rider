#pragma once
#include "PluginProcessor.h"
#include "UI/HardwareLookAndFeel.h"

class RangeBand final : public juce::Component {
public:
    explicit RangeBand(VocalBalanceAudioProcessor& p) : processor(p) {}
    void paint(juce::Graphics&) override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
private:
    VocalBalanceAudioProcessor& processor;
    juce::RangedAudioParameter* dragging = nullptr;
    bool lowerHandle = false;
};
class VocalBalanceAudioProcessorEditor final : public juce::AudioProcessorEditor, private juce::Timer {
public:
    explicit VocalBalanceAudioProcessorEditor(VocalBalanceAudioProcessor&);
    ~VocalBalanceAudioProcessorEditor() override;
    void paint(juce::Graphics&) override;
    void resized() override;
private:
    void timerCallback() override;
    void buildPanel();
    void loadPreset();
    void savePreset();
    void setMode(int);
    VocalBalanceAudioProcessor& processor;
    HardwareLookAndFeel look;
    juce::Image panel;
    RangeBand range;
    std::array<juce::Slider, 6> controls;
    std::array<std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment>, 6> attachments;
    juce::ComboBox presets;
    juce::TextButton attack{"Slow"};
    std::array<juce::TextButton, 3> modeButtons;
    std::array<juce::TextButton, 6> toolbar;
    juce::ToggleButton bypass{"Byp"};
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> bypassAttachment;
    std::unique_ptr<juce::FileChooser> chooser;
    juce::ValueTree bankA, bankB;
    bool comparingB = false;
    juce::TooltipWindow tooltips{this, 700};
};
