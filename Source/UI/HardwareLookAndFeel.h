#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
class HardwareLookAndFeel final : public juce::LookAndFeel_V4 {
public:
    static void drawFaderCap(juce::Graphics&, juce::Rectangle<float>, bool rider);
    static void drawFaderTrack(juce::Graphics&, float centre, float top, float bottom);
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    void drawLinearSlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider::SliderStyle, juce::Slider&) override;
    void drawRotarySlider(juce::Graphics&, int, int, int, int, float, float, float, juce::Slider&) override;
};
